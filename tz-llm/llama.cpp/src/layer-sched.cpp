#include "pipeline.h"
#include "ggml.h"
#include "io-frontend.h"
#ifdef LLAMA_USE_CHCORE_API
#include <chcore/llm.h>
#endif
#include <atomic>
#include <cstdio>

// Self-contained lock-free ring buffer for LayerScheduler::step()'s idle
// path (2026-08-09). Mirrors io-backend.cpp's dbg_log_push/dump pattern,
// but that one lives in io-backend.cpp, which is CA-only (CMakeLists.txt:
// only compiled when NOT LLAMA_CHCORE_API -- the TA build uses
// alloc-stage-chcore.cpp instead and never sees io-backend.cpp's symbols
// at all). This file compiles into BOTH the TA and CA builds, so it needs
// its own copy rather than calling into io-backend.cpp's (confirmed on
// hardware: cross-referencing it produced `undefined reference to
// dbg_log_push_idle` when linking every TA-side executable). Push is a
// plain struct write (no I/O) -- safe at the millions-of-calls/run rate
// this fires at; only dbg_log_idle_dump() (called on demand, e.g. from a
// SIGUSR1 handler) ever touches stdout.
struct idle_log_entry {
    int io_cnt;
    int on_fly_cnt;
    size_t alloc_sz, io_sz, decrypt_sz;
};
#define IDLE_LOG_SIZE 32
static idle_log_entry idle_log[IDLE_LOG_SIZE];
static std::atomic<uint64_t> idle_log_idx{0};

static inline void dbg_log_push_idle(int io_cnt, int on_fly_cnt, size_t alloc_sz, size_t io_sz, size_t decrypt_sz) {
    uint64_t i = idle_log_idx.fetch_add(1, std::memory_order_relaxed);
    idle_log_entry &e = idle_log[i % IDLE_LOG_SIZE];
    e.io_cnt = io_cnt;
    e.on_fly_cnt = on_fly_cnt;
    e.alloc_sz = alloc_sz;
    e.io_sz = io_sz;
    e.decrypt_sz = decrypt_sz;
}

void dbg_log_idle_dump(void) {
    uint64_t total = idle_log_idx.load(std::memory_order_relaxed);
    uint64_t start = total > IDLE_LOG_SIZE ? total - IDLE_LOG_SIZE : 0;
    printf("[DBG_LOG_IDLE_DUMP] last %llu of %llu step()-idle events:\n",
        (unsigned long long)(total - start), (unsigned long long)total);
    for (uint64_t i = start; i < total; i++) {
        idle_log_entry &e = idle_log[i % IDLE_LOG_SIZE];
        printf("  #%llu io_cnt=%d on_fly_cnt=%d alloc_sz=%zu io_sz=%zu decrypt_sz=%zu\n",
            (unsigned long long)i, e.io_cnt, e.on_fly_cnt, e.alloc_sz, e.io_sz, e.decrypt_sz);
    }
}

std::pair<std::shared_ptr<Pipeline>, std::shared_ptr<Task>> LayerScheduler::get_task(layer_queue_t &queue, void *arg)
{
    while (true) {
        if (queue.empty()) {
            return std::make_pair(nullptr, nullptr);
        }
        auto pipeline = queue.top();
        GGML_ASSERT(!pipeline->is_finished());
        auto task = pipeline->get_current_stage()->get_task(arg);
        if (task.second) {
            queue.pop();
        }
        return std::make_pair(pipeline, task.first);
    }
}

pid_t main_tid = -1;

#ifdef LLAMA_USE_CHCORE_API
std::atomic<int> cma_index_counter;
thread_local int my_cma_index = -1;

bool is_pipelining = false;

int get_cma_index(void) {
    if (my_cma_index == -1) {
        // TZASC_NR_MODEL (not TZASC_NR): index TZASC_NR-1 is reserved for
        // NPU real-weight scratch buffers, see chcore/llm.h.
        my_cma_index = cma_index_counter.fetch_add(1) % TZASC_NR_MODEL;
    }
    return my_cma_index;
}
#else
int get_cma_index(void) {
    return 0;
}
#endif

int io_cnt = 0;

std::mutex io_lock;
bool LayerScheduler::step(void) {
extern bool is_strawman;
if (!is_strawman) {
    GGML_ASSERT(main_tid != -1);
    if (gettid() == main_tid) {
        auto entry = io_try_get();
        if (entry.has_value()) {
            io_cnt--;
            auto pipeline = entry->pipeline;
            auto task = entry->task;
            if (pipeline->get_current_stage()->submit(task)) {
                pipeline->finish_stage();
                if (!pipeline->is_finished()) {
                    enqueue(pipeline);
                }
            }
            // return false;
        }
    }

    std::pair<std::shared_ptr<Pipeline>, std::shared_ptr<Task>> res;
    {
        std::lock_guard<std::mutex> _(lock);

        while (true) {
            if (gettid() == main_tid && io_cnt <= 32) {
                res = get_task(io, NULL);
                if (res.first) {
                    io_cnt++;
                    break;
                }
            }
            res = get_task(decrypt, NULL);
            if (res.first) break;
            GGML_ASSERT(main_tid != -1);
#ifdef LLAMA_USE_CHCORE_API
            res = get_task(alloc, (void *)(long)get_cma_index());
#else
            if (gettid() == main_tid) {
                res = get_task(alloc, (void *)(long)get_cma_index());
            }
#endif
            if (res.first) break;
            /* TEMP DIAGNOSTIC: all 3 stage queues empty AND no in-flight IO
             * ever completes -- pairs with the [TZLLM_TRACE] SMC push/wake
             * trace, which showed push_pages succeeding hundreds of times
             * then abruptly stopping (~178MB into the model, same point
             * with or without NPU offload) while the io_rpc()/x2=4 poll
             * loop kept spinning forever after. Logging the actual queue
             * sizes + io_cnt + on_fly_cnt here pinpoints which counter/queue
             * is the one that got stuck, instead of guessing from outside. */
            if (gettid() == main_tid) {
                extern int on_fly_cnt;
                // RING BUFFER UPDATE (2026-08-09): this fires every time all
                // 3 stage queues are empty (the common/expected state
                // between real work items, millions of times per run).
                // Throttled printf (previous version, kept in git history)
                // still cost real UART I/O on every Nth call and contributed
                // to the RCU-stall/soft-lockup cascade documented in
                // tc_client_driver.c. Pushing into the existing lock-free
                // ring buffer defined at the top of this file (self-
                // contained, see its comment for why) is a plain struct
                // write, not I/O -- zero steady-state console cost, full
                // recent history still available on demand via
                // dbg_log_idle_dump() (SIGUSR1).
                dbg_log_push_idle(io_cnt, on_fly_cnt, alloc.size(), io.size(), decrypt.size());
            }
            return false;
        }
    }

    auto pipeline = res.first;
    auto task = res.second;
    GGML_ASSERT(pipeline && task);

    task->step();
    if (pipeline->get_current_stage()->submit(task)) {
        // TEMP DIAGNOSTIC: chasing the fixed-point stall (Bug #2
        // continuation, STATUS.md) -- confirm the stage transition
        // (alloc->io->decrypt->finished) actually happens for every
        // pipeline whose last block just completed, since AllocStage's
        // own ALLOC_TRACE only shows submit()'s is_done bit, not whether
        // the pipeline successfully re-enters a queue afterward.
#ifdef LLAMA_USE_CHCORE_API
        extern std::mutex alloc_trace_print_mtx;
        {
            std::lock_guard<std::mutex> _p(alloc_trace_print_mtx);
            printf("[ALLOC_TRACE] finish_stage+enqueue pipeline=%p sched_info=%p\n",
                (void *)pipeline.get(), pipeline->get_sched_info());
            fflush(stdout);
        }
#endif
        pipeline->finish_stage();
        if (!pipeline->is_finished()) {
            enqueue(pipeline);
        }
    }

    return true;
} else {
    bool is_io = false;

    {
        std::lock_guard<std::mutex> _(io_lock);
        auto entry = io_try_get();
        if (entry.has_value()) {
            auto pipeline = entry->pipeline;
            auto task = entry->task;
            if (pipeline->get_current_stage()->submit(task)) {
#ifdef LLAMA_USE_CHCORE_API
                extern std::mutex alloc_trace_print_mtx;
                {
                    std::lock_guard<std::mutex> _p(alloc_trace_print_mtx);
                    printf("[ALLOC_TRACE] (strawman io_try_get) finish_stage+enqueue pipeline=%p sched_info=%p\n",
                        (void *)pipeline.get(), pipeline->get_sched_info());
                    fflush(stdout);
                }
#endif
                pipeline->finish_stage();
                if (!pipeline->is_finished()) {
                    enqueue(pipeline);
                }
            }
            return true;
        }
    }

    std::pair<std::shared_ptr<Pipeline>, std::shared_ptr<Task>> res;
    {
        std::lock_guard<std::mutex> _(lock);
        GGML_ASSERT(main_tid != -1);
        while (true) {
#ifdef LLAMA_USE_CHCORE_API
            res = get_task(alloc, (void *)(long)get_cma_index());
#else
            if (gettid() == main_tid) {
                res = get_task(alloc, (void *)(long)get_cma_index());
            }
#endif
            if (res.first) break;
            res = get_task(io, NULL);
            if (res.first) {
                is_io = true;
                break;
            }
            
            res = get_task(decrypt, NULL);
            if (res.first) break;
            return false;
        }
    }

    auto pipeline = res.first;
    auto task = res.second;
    GGML_ASSERT(pipeline && task);

    if (is_io) io_lock.lock();
    task->step();
    if (is_io) io_lock.unlock();
    if (pipeline->get_current_stage()->submit(task)) {
#ifdef LLAMA_USE_CHCORE_API
        // TEMP DIAGNOSTIC: same as the non-strawman call site above --
        // this is the ACTUAL call site exercised when testing with -s 1
        // (strawman), which is what every reproduction of the Bug #2
        // stall this session has used. The first instrumented build put
        // this trace only in the non-strawman branch and never fired.
        extern std::mutex alloc_trace_print_mtx;
        {
            std::lock_guard<std::mutex> _p(alloc_trace_print_mtx);
            printf("[ALLOC_TRACE] (strawman) finish_stage+enqueue pipeline=%p sched_info=%p is_io=%d\n",
                (void *)pipeline.get(), pipeline->get_sched_info(), is_io);
            fflush(stdout);
        }
#endif
        pipeline->finish_stage();
        if (!pipeline->is_finished()) {
            enqueue(pipeline);
        }
    }

    return !is_io;
}
}

void LayerScheduler::enqueue(std::shared_ptr<Pipeline> pipeline)
{
    auto current_stage = pipeline->get_current_stage();
    GGML_ASSERT(current_stage);

    std::lock_guard<std::mutex> _(lock);
    if (std::dynamic_pointer_cast<AllocStage>(current_stage)) {
        alloc.push(pipeline);
    } else if (std::dynamic_pointer_cast<IOStage>(current_stage)) {
        io.push(pipeline);
    } else if (std::dynamic_pointer_cast<DecryptStage>(current_stage)) {
        decrypt.push(pipeline);
    } else {
        GGML_ASSERT(false);
    }
}