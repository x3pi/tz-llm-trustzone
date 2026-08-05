#pragma once

#include <sys/types.h>
#include <unistd.h>
#include "my_assert.h"
#include <cstdio>
#include <pthread.h>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <atomic>
#include <mutex>
#include <cstring>

#define TZ_LLM_MEASURE

#define DUMMY_WEIGHT 0

#define ENABLE_PIPELINE 1

#define CMD_QUEUE_SHM_NAME "/cmd_queue_shm"
#define CMD_QUEUE_SHM_SIZE (256 * 4096)

#define IO_BUFFER_SHM_NAME "/io_buffer_shm"
#define IO_BUFFER_SHM_SIZE (600 * 1024 * 1024)
#define IO_BUFFER_NR (2)

#define IO_SEG_NR (10)

struct io_seg {
    size_t off;
    size_t len;
};

struct io_segs {
    int seg_nr;
    io_seg io_segs[4];
};

struct io_task {
    int cma_index;
    int entry_index;
    unsigned long entry_offset; // see pipeline.h's cma_indexes comment
    size_t len;
    struct io_seg io_seg;
    void *pipeline;

    bool is_measurement;
    double ttft;
    double decoding_thpt;
};

struct io_result {
    void *pipeline;
};

typedef enum {
    NPU_TASK_OP_CREATE_MEM_FROM_FD,
    NPU_TASK_OP_DESTROY_MEM,
    NPU_TASK_OP_MATMUL_CREATE,
    NPU_TASK_OP_MATMUL_DESTROY,
    NPU_TASK_OP_MATMUL_SET_CORE_MASK,
    NPU_TASK_OP_MATMUL_SET_IO_MEM,
    NPU_TASK_OP_MATMUL_RUN,
} npu_task_op_t;

struct npu_task {
    npu_task_op_t op;
};

struct npu_result {
    npu_task_op_t op;
};

typedef enum {
    PAGE_TASK_OP_ALLOC,
    PAGE_TASK_OP_FREE,
} page_task_op_t;

struct page_task {
    page_task_op_t op;
    unsigned long entry_begin;
    unsigned long entry_end;
    size_t off;
};

struct page_result {
    unsigned long entry_begin;
    unsigned long entry_end;
    size_t off;
};

// BUG FIX: ring_buffer instances (io_tasks/io_results inside
// all_ring_buffer) live in memory mmap'd from the TZ driver's shared page -
// genuinely shared between the Normal World CA process (glibc/Linux) and
// the Secure World TA (ChCore, chcore-libc/musl). A previous fix for a real
// produce() reordering race (see below) serialized access with a
// std::mutex, on the mistaken assumption that this buffer is only ever
// touched by multiple threads of a single process. It is not: io_launch()
// (TA side) and io_step() (CA side) both lock the SAME mtx bytes in this
// shared struct. std::mutex is a pthread_mutex_t with default
// PTHREAD_PROCESS_PRIVATE semantics, and glibc's NPTL and musl's pthread
// implementation use *different* internal layouts/algorithms for that
// struct - one side's lock()/unlock() writes bytes the other side's libc
// doesn't recognize as valid state, eventually tripping glibc's own
// consistency check ("Fatal glibc error: pthread_mutex_lock.c:94:
// assertion failed: mutex->__data.__owner == 0"). Confirmed on hardware:
// this crash reproduces late in a run (~400-1200+ produce/consume cycles
// in, varying between runs) - consistent with a cross-libc byte-level race,
// not a deterministic bug. Fix: use a raw spinlock built purely from
// std::atomic (CAS loop), which only relies on cache-coherent hardware
// atomic instructions (LDXR/STXR / LSE) - no OS futex syscall, no per-libc
// thread/owner bookkeeping - so it's actually valid across this world
// boundary, unlike any OS-level mutex.
struct raw_spinlock {
    std::atomic<int> locked{0};

    void lock(void) {
        int expected;
        do {
            expected = 0;
        } while (!locked.compare_exchange_weak(expected, 1,
            std::memory_order_acquire, std::memory_order_relaxed));
    }

    void unlock(void) {
        locked.store(0, std::memory_order_release);
    }
};

template<typename T, int BUFFER_SIZE>
struct ring_buffer {
    T buffer[BUFFER_SIZE];
    std::atomic<int> head;
    std::atomic<int> tail;
    std::atomic<int> count;
    // BUG FIX (original race, still applicable): this was a lock-free ring
    // buffer where produce() advances `head` (claiming a slot) via CAS
    // *before* memcpy-ing the item into it, and only increments `count`
    // (which gates consumer visibility) *after* the memcpy. With a single
    // producer this is fine, but io_launch()/record_measure() (TA side) and
    // io_step() (CA side) can each have multiple concurrent callers - if
    // producer B's CAS+memcpy+count++ all complete before producer A's
    // memcpy (even though A's CAS claimed an *earlier* slot), a consumer
    // can see count reflect B's write and dequeue in tail order, landing on
    // A's still-unwritten slot. That slot's backing memory is the driver's
    // shared g_llm_shm region, which reads as all-zero until first written -
    // so the consumer gets a zeroed-out io_task (cma_index=0,
    // entry_index=0, len=0, is_measurement=false) that looks like a
    // legitimate but empty IO request, and get_buf()'s mmap(..., len=0,
    // ...) fails with EINVAL. Confirmed on hardware: this exact zeroed-task
    // signature, appearing late in a run (more concurrent producer/consumer
    // churn by then). Fix: serialize produce()/consume() with the
    // cross-world-safe raw_spinlock above.
    raw_spinlock mtx;

    void init(void) {
        head = tail = count = 0;
    }

    ring_buffer(void): head(0), tail(0), count(0) {}
    int produce(const T *item) {
        std::lock_guard<raw_spinlock> _(mtx);
        GGML_ASSERT(this->count.load() != BUFFER_SIZE);
        int _head = this->head.load();
        memcpy(this->buffer + _head, item, sizeof(T));
        this->head.store((_head + 1) % BUFFER_SIZE);
        this->count++;
        return 0;
    }

    int consume(T *item) {
        std::lock_guard<raw_spinlock> _(mtx);
        if (this->count.load() == 0) {
            return -1;
        }
        int _tail = this->tail.load();
        memcpy(item, this->buffer + _tail, sizeof(T));
        this->tail.store((_tail + 1) % BUFFER_SIZE);
        this->count--;
        return 0;
    }
};

const int IO_BUFFER_SIZE = 10240;
const int NPU_BUFFER_SIZE = 128;
const int PAGE_BUFFER_SIZE = 128;

// PRODUCTION GAP FOUND: this whole session's debugging relied on the TA's
// own UART console for the generated answer text -- but that's a raw
// hardware serial line shared with every other concurrently-printing
// thread/kernel subsystem, and repeatedly corrupted the answer text itself
// (START/END markers survived, the text between them consistently came
// through empty) across many independent test runs. `fake_ca.cpp` (the
// Normal-World CA) never actually reads or relays the generated text at
// all today -- it's a pure SMC/IO relay pump with zero awareness of
// inference results. There was NO reliable way for a real caller to get
// the answer out. Fixed by giving the TA a place to publish the final
// text into this ALREADY-SHARED, ALREADY-WORKING command-queue page (the
// same one `cache_p`/`prompt`/`n` etc. use for the CA->TA direction) for
// the CA to read back and print through its own plain stdout, which
// (unlike the UART) reliably reaches a redirected log file untouched.
const size_t FINAL_ANSWER_MAX = 8192;

struct all_ring_buffer {
    char io_model_path[256];
    char cache_p[256];
    char inner_model_path[256];
    char prompt[256];
    char n[256];
    bool is_strawman;

    char final_answer[FINAL_ANSWER_MAX];
    std::atomic<bool> final_answer_ready;

    ring_buffer<io_task, IO_BUFFER_SIZE> io_tasks;
    ring_buffer<io_result, IO_BUFFER_SIZE> io_results;
    // ring_buffer<npu_task, NPU_BUFFER_SIZE> npu_tasks;
    // ring_buffer<npu_result, NPU_BUFFER_SIZE> npu_results;
    // ring_buffer<page_task, PAGE_BUFFER_SIZE> page_tasks;
    // ring_buffer<page_result, PAGE_BUFFER_SIZE> page_results;

    void init(void) {
        GGML_ASSERT(sizeof(all_ring_buffer) <= CMD_QUEUE_SHM_SIZE);
        memset(io_model_path, 0, sizeof(io_model_path));
        memset(cache_p, 0, sizeof(cache_p));
        memset(inner_model_path, 0, sizeof(inner_model_path));
        memset(prompt, 0, sizeof(prompt));
        memset(n, 0, sizeof(n));
        memset(final_answer, 0, sizeof(final_answer));
        final_answer_ready = false;
        io_tasks.init();
        io_results.init();
        // npu_tasks.init();
        // npu_results.init();
        // page_tasks.init();
        // page_results.init();
    }
};

static inline int64_t get_micro(void) {
    sizeof(all_ring_buffer);
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}


static_assert(sizeof(all_ring_buffer) <= CMD_QUEUE_SHM_SIZE, "sizeof(all_ring_buffer) > CMD_QUEUE_SHM_SIZE");

static inline void *shm_client(const char *shm_name, size_t shm_size) {
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    GGML_ASSERT(shm_fd != -1);
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    GGML_ASSERT(ptr != MAP_FAILED);
    return ptr;
}

static inline void *shm_server(const char *shm_name, size_t shm_size) {
    shm_unlink(shm_name);
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    GGML_ASSERT(shm_fd != -1);
    GGML_ASSERT(ftruncate(shm_fd, shm_size) == 0);
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    GGML_ASSERT(ptr != MAP_FAILED);
    return ptr;
}
