#include "io.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <iostream>
#include <cstring>
#include <memory>
#include <optional>
#include <libaio.h>
#include <queue>
#include "my_assert.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstdint>
#include <atomic>

struct aio_task {
    io_context_t ctx;
    void *pipeline;
    void *cma_buf;
    void *read_buf;
    size_t len;
    size_t off;
    aio_task(io_context_t ctx, void *pipeline, void *cma_buf, void *read_buf, size_t len, size_t off = 0)
        : ctx(ctx), pipeline(pipeline), cma_buf(cma_buf), read_buf(read_buf), len(len), off(off) {}
};

#define IO_BLK_SIZE (2 << 20)

static std::queue<std::shared_ptr<aio_task>> tasks;
static std::mutex tasks_mtx;
static int fd, tzd_fd;

// Non-intrusive diagnostic log: pure in-memory writes (no syscalls, no
// formatting) on the hot path so it doesn't perturb the timing-sensitive
// race we're chasing, unlike live printf() (confirmed on hardware to make
// the bug manifest earlier/worse, especially from secure-world context).
// Contents are only formatted and printed ONCE, right when get_buf()'s
// mmap() actually fails, giving the exact sequence of events leading up
// to the failure without touching steady-state overhead.
struct dbg_log_entry {
    int kind; // 0 = io_step consume, 1 = wait_io complete, 2 = get_buf call
    int is_measurement;
    int cma_index;
    int entry_index;
    size_t len;
    size_t off;
    void *pipeline;
};
#define DBG_LOG_SIZE 64
static dbg_log_entry dbg_log[DBG_LOG_SIZE];
static std::atomic<uint64_t> dbg_log_idx{0};

static inline void dbg_log_push(int kind, int is_measurement, int cma_index, int entry_index, size_t len, size_t off, void *pipeline) {
    uint64_t i = dbg_log_idx.fetch_add(1, std::memory_order_relaxed);
    dbg_log_entry &e = dbg_log[i % DBG_LOG_SIZE];
    e.kind = kind;
    e.is_measurement = is_measurement;
    e.cma_index = cma_index;
    e.entry_index = entry_index;
    e.len = len;
    e.off = off;
    e.pipeline = pipeline;
}

void dbg_log_dump(void) {
    uint64_t total = dbg_log_idx.load(std::memory_order_relaxed);
    uint64_t start = total > DBG_LOG_SIZE ? total - DBG_LOG_SIZE : 0;
    printf("[DBG_LOG_DUMP] last %llu of %llu events (kind: 0=io_step 1=wait_io 2=get_buf):\n",
        (unsigned long long)(total - start), (unsigned long long)total);
    for (uint64_t i = start; i < total; i++) {
        dbg_log_entry &e = dbg_log[i % DBG_LOG_SIZE];
        printf("  #%llu kind=%d is_meas=%d cma_index=%d entry_index=%d len=%zu off=%zu pipeline=%p\n",
            (unsigned long long)i, e.kind, e.is_measurement, e.cma_index, e.entry_index, e.len, e.off, e.pipeline);
    }
}
// static const char *model_path = "/data/ssd/tinyllama-1.1b-chat-v1.0.Q8_0.gguf";
#if DUMMY_WEIGHT
static void *global_read_buf;
static size_t global_read_buf_len;
#endif

struct llm_client_op_pages {
	int cma_index;
	int entry_index;
	unsigned long size;
};

#define DEVICE_NAME "/dev/tc_ns_client"
#define TC_NS_CLIENT_IOC_MAGIC  't'
#define LLM_CLIENT_IOCTL_SET_PAGES \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 27, struct llm_client_op_pages)

static std::queue<io_context_t> ctxs;
static std::mutex ctxs_mtx;

io_context_t get_ctx(void) {
    std::lock_guard<std::mutex> lock(ctxs_mtx);
    if (ctxs.empty()) {
        io_context_t new_ctx = NULL;
        GGML_ASSERT(io_setup(1, &new_ctx) == 0);
        ctxs.push(new_ctx);
    }
    auto ctx = ctxs.front();
    ctxs.pop();
    return ctx;
}

void put_ctx(io_context_t ctx) {
    std::lock_guard<std::mutex> lock(ctxs_mtx);
    ctxs.push(ctx);
}

// BUG FIX: llm_client_mmap() (kernel side) reads which cma_index/entry_index
// to map from file->private_data, which the preceding SET_PAGES ioctl call
// sets on the SAME shared `tzd_fd`. Up to 4 ca_thread pthreads all call
// io_step() -> get_buf() concurrently on this one global fd - without a
// lock, thread B's SET_PAGES could overwrite thread A's private_data between
// A's ioctl and A's mmap, causing A to map B's (wrong-sized) entry and hit
// mmap()'s GGML_ASSERT(addr != MAP_FAILED) (confirmed on hardware: this
// exact assert fired after the on_fly_io_thread per-CPU fix let real
// multi-thread parallelism reach this code path for the first time).
static std::mutex get_buf_mtx;

static void *get_buf(int cma_index, int entry_index, size_t len) {
    if (cma_index == -1)
        return NULL;
    std::lock_guard<std::mutex> _(get_buf_mtx);
    struct llm_client_op_pages index = {
        .cma_index = cma_index,
        .entry_index = entry_index,
    };
    int ret = ioctl(tzd_fd, LLM_CLIENT_IOCTL_SET_PAGES, &index);
    GGML_ASSERT(ret >= 0);
    dbg_log_push(2, -1, cma_index, entry_index, len, 0, NULL);

    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, tzd_fd, 0);
    if (addr == MAP_FAILED) {
        int saved_errno = errno;
        dbg_log_dump();
        printf("[DBG_MMAP_FAIL] cma_index=%d entry_index=%d len=%zu errno=%d (%s)\n",
            cma_index, entry_index, len, saved_errno, strerror(saved_errno));
    }
    GGML_ASSERT(addr != MAP_FAILED);

    return addr;
}

static void launch_io(void *dst, int fd, const io_seg &io_seg, void *pipeline) {
#if DUMMY_WEIGHT
    if (io_seg.len > global_read_buf_len) {
        printf("[warn] extend global read buffer from %ldB to %ldB\n", global_read_buf_len, io_seg.len);
        // munmap(global_read_buf, global_read_buf_len);
        global_read_buf = mmap(NULL, io_seg.len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        global_read_buf_len = io_seg.len;
    }
    void *read_buf = global_read_buf;
#else
    void *read_buf = mmap(NULL, io_seg.len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    GGML_ASSERT(read_buf != MAP_FAILED);

    auto task = std::make_shared<aio_task>(get_ctx(), pipeline, dst, read_buf, io_seg.len, io_seg.off);
    
    struct iocb *cbs[1];
    iocb cb;
    memset(&cb, 0, sizeof(cb));
    io_prep_pread(&cb, fd, read_buf, io_seg.len, io_seg.off);
    cbs[0] = &cb;

    GGML_ASSERT(io_submit(task->ctx, 1, cbs) == 1);
    {
        std::lock_guard<std::mutex> lock(tasks_mtx);
        tasks.push(task);
    }
}

static std::mutex wait_io_mtx;

static void *wait_io(void) {
    std::lock_guard<std::mutex> wait_lock(wait_io_mtx);
    struct io_event event;
    timespec timeout = { .tv_sec = 0, .tv_nsec = 0 };
    std::shared_ptr<aio_task> task;
    {
        std::lock_guard<std::mutex> lock(tasks_mtx);
        if (tasks.empty()) return NULL;
        task = tasks.front();
    }
    int ret = io_getevents(task->ctx, 1, 1, &event, &timeout);
    GGML_ASSERT(ret >= 0);
    if (ret > 0) {
        GGML_ASSERT((int)event.res >= 0);
#if not(DUMMY_WEIGHT)
        dbg_log_push(1, -1, -1, -1, task->len, task->off, task->pipeline);
        memcpy(task->cma_buf, task->read_buf, task->len);
        munmap(task->read_buf, task->len);
        // BUG FIX: task->cma_buf (from get_buf()'s mmap() of /dev/tc_ns_client)
        // was never unmapped anywhere in this file - only the anonymous
        // staging buffer above was. Every one of the ~275 alloc/io chunks for
        // a 1.1GB model leaked one mmap'd VMA; confirmed on hardware this
        // eventually makes a LATER get_buf() call's own mmap() fail with
        // MAP_FAILED (observed after ~178 chunks / 748MB). The underlying
        // physical CMA page outlives this mapping (owned by the TA's
        // alloc/pop lifecycle, not this mapping), so unmapping here is safe -
        // we're done with cma_buf once the real data has been copied into it.
        munmap(task->cma_buf, task->len);
#endif
        {
            std::lock_guard<std::mutex> lock(tasks_mtx);
            if (!tasks.empty() && tasks.front() == task) {
                tasks.pop();
            }
        }
        put_ctx(task->ctx);
        return task->pipeline;
    }
    return NULL;
}

#define IO_TEST_FILE "/data/ssd/Meta-Llama-3-8B-Instruct.Q8_0.gguf"
#define IO_TEST_FILE_SIZE (8UL << 30)
#define IO_PRE_LAUNCH_CNT (16)

void io_init(const char *model_path) {
    printf("backend %s %d %s\n", __func__, __LINE__, model_path);
    tzd_fd = open(DEVICE_NAME, O_RDWR);
    GGML_ASSERT(tzd_fd > 0);

    fd = open(model_path, O_RDONLY | O_DIRECT);
    GGML_ASSERT(fd != -1);

#if DUMMY_WEIGHT
    global_read_buf = mmap(NULL, IO_BLK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    global_read_buf_len = IO_BLK_SIZE;
    if (0) {
        printf("begin io test\n");
        auto start = get_micro();
        fd = open(IO_TEST_FILE, O_RDONLY | O_DIRECT);
        int all = 0, wait = 0;
        for (size_t i = 0; i < IO_TEST_FILE_SIZE; i += IO_BLK_SIZE) {
            struct io_seg io_seg = {
                .off = i,
                .len = IO_BLK_SIZE,
            };
            launch_io(global_read_buf, fd, io_seg, (void *)1);
            if (++all >= IO_PRE_LAUNCH_CNT) {
                while (wait_io());
                ++wait;
            }
        }
        for (; wait < all; wait++) {
            while (wait_io());
        }
        printf("io test %ld us thpt %.2f GB/s\n", get_micro() - start, 0.001f * IO_TEST_FILE_SIZE / (get_micro() - start));
    }
#endif
}

static void write_measurement(const io_task &task) {
    // Use POSIX shared memory object under /dev/shm
    const char *shm_name = "/current_measure";  // results in /dev/shm/current_measure
    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("shm_open");
        return;
    }

    FILE *fp = fdopen(fd, "w");
    if (!fp) {
        perror("fdopen");
        close(fd);
        return;
    }

    // Overwrite with the latest measurement (no append)
    // Exact format requested by user
    fprintf(fp, "ttft: %.2f\ndecoding_thpt: %.2f\n", task.ttft, task.decoding_thpt);
    fflush(fp);
    fsync(fd);
    fclose(fp); // also closes fd
}

void io_step(all_ring_buffer *task_queue) {
    io_task task;
    while (task_queue->io_tasks.consume(&task) == 0) {
        dbg_log_push(0, task.is_measurement, task.cma_index, task.entry_index, task.len, task.io_seg.off, task.pipeline);
        if (task.is_measurement) {
            write_measurement(task);
            return;
        } else {
            void *buf = get_buf(task.cma_index, task.entry_index, task.len);
            launch_io(buf, fd, task.io_seg, task.pipeline);
        }
    }
    void *pipeline;
    while (pipeline = wait_io()) {
        io_result result = {
            .pipeline = pipeline
        };
        task_queue->io_results.produce(&result);
    }
}
