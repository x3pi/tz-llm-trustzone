#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include <stdio.h>
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <optional>
#include <cstring>
#include <vector>
#include "crypto.h"
#include <unistd.h>
#include <mapping.h>
#include "io-frontend.h"
#include <mutex>
#include <thread>
#include "pipeline.h"

// NOTE: the canonical definition lives in ggml-rknpu-re.cpp (part of
// libggml.so). Defining a SEPARATE `bool is_strawman = false;` here (part
// of libllama.so) created two independent globals across the shared
// library boundary - setting one (e.g. via ggml_backend_rknpure_set_strawman)
// never updated the other, so ggml_backend_rknpure_supports_op() kept
// seeing its own stale copy and dispatched to the buggy RKNPURE compute
// path even when strawman mode was supposedly enabled everywhere else.
extern bool is_strawman;

struct param_tensor_desc {
    ggml_tensor *tensor;
    std::shared_ptr<Pipeline> pipeline;

    param_tensor_desc(
        ggml_tensor *tensor,
        std::shared_ptr<Pipeline> pipeline
    ): tensor(tensor), pipeline(pipeline) {}

    param_tensor_desc(const param_tensor_desc&) = delete;
    param_tensor_desc& operator=(const param_tensor_desc&) = delete;
    param_tensor_desc(param_tensor_desc&&) = delete;
    param_tensor_desc& operator=(param_tensor_desc&&) = delete;
};

std::map<ggml_tensor *, std::shared_ptr<param_tensor_desc>> param_tensors;
Scheduler *sched = new LayerScheduler();
std::mutex use_mtx;

static std::pair<std::string, int> parse_name(const char *tensor_name) {
    if (strncmp("blk", tensor_name, 3) == 0) {
        const char *layer_begin = strchr(tensor_name, '.');
        GGML_ASSERT(layer_begin != NULL);
        layer_begin++;
        const char *layer_end = strchr(layer_begin, '.');
        char layer_str[layer_end - layer_begin];
        memcpy(layer_str, layer_begin, layer_end - layer_begin);
        layer_str[layer_end - layer_begin] = 0;
        int layer = atoi(layer_str);
        std::string name(layer_end + 1);
        return { name, layer };
    } else {
        std::string name(tensor_name);
        return { name, strstr(tensor_name, "output") ? 999 : -1 };
    }
}

int64_t use_wait_time;
int64_t use_wait_cpu_time;
int64_t base_time;

extern "C" {

void use_param_tensor(
    ggml_tensor *tensor,
    int ith
) {
#ifdef TZ_LLM_MEASURE
    auto start = get_micro();
#endif
    // std::lock_guard<std::mutex> _(use_mtx);
    use_mtx.lock();
    auto desc_iter = param_tensors.find(tensor);
    GGML_ASSERT(desc_iter != param_tensors.end());
    use_mtx.unlock();
    auto pipeline = desc_iter->second->pipeline;
    while (!pipeline->is_finished()) {
#ifdef TZ_LLM_MEASURE
        auto start = get_micro();
#endif
        bool is_cpu = sched->step();
#ifdef TZ_LLM_MEASURE
        if (ith == 0 && is_cpu) {
            use_wait_cpu_time += get_micro() - start;
        }
#endif
    }
    // BUG FIX: this used to be gated on `if (ith == 0)`, meaning only thread
    // 0 wrote tensor->data. ggml_compute_forward_use_param() (ggml.c) reads
    // src0->data right after this function returns, on EVERY thread (ith
    // 0..3), with no barrier separating "is_finished() observed true" from
    // "thread 0 has actually executed the assignment below". Threads with
    // ith != 0 could see is_finished()==true and return before thread 0
    // reached this line, then read tensor->data while it was still whatever
    // uninitialized value the CMA page had before loading (observed as long
    // runs of 0xFF bytes in the first ~16 bytes of most weight tensors).
    // get_final_msg() is mutex-protected and idempotent, so it's safe for
    // every thread to just compute and set it themselves instead of relying
    // on cross-thread visibility of a single thread's write.
    tensor->data = pipeline->get_final_msg();
    // DISABLED (2026-08-08): this printed unconditionally on every tensor
    // use, every thread (ith 0..3), every forward pass -- for a 22-layer
    // model that's thousands of printf calls per token, each one a
    // synchronous write over this board's slow 1.5Mbaud UART console.
    // Confirmed live on hardware this session: this trace (and the sibling
    // step()-idle trace in layer-sched.cpp) still firing millions of times
    // during a long-running session correlates with the RCU-stall/soft-
    // lockup cascade documented in tc_client_driver.c's smc_call_cpu_resume().
    // Its diagnostic job (confirming tensor->data is populated before use,
    // see the BUG FIX comment above) is done; the underlying bug it helped
    // find is fixed. Left the dbg byte-read computation commented out too
    // so it's a one-line re-enable if ever needed again, not a rewrite.
    // {
    //     unsigned char *dbg = (unsigned char *)tensor->data;
    //     printf("[DBG_USE] ith=%d name=%s data=%p bytes=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
    //         ith, tensor->name, tensor->data,
    //         dbg[0], dbg[1], dbg[2], dbg[3], dbg[4], dbg[5], dbg[6], dbg[7],
    //         dbg[8], dbg[9], dbg[10], dbg[11], dbg[12], dbg[13], dbg[14], dbg[15]);
    // }
#ifdef TZ_LLM_MEASURE
    if (ith == 0)
        use_wait_time += get_micro() - start;
#endif
}

}

void reset_param_tensor(void) {
    for (auto &desc: param_tensors) {
        desc.second->pipeline->rollback();
    }
    for (auto &desc: param_tensors) {
        desc.second->pipeline->get_current_stage()->start(NULL);
        sched->enqueue(desc.second->pipeline);
    }
}

#ifdef LLAMA_USE_CHCORE_API
extern "C" void usys_yield(void);
#endif

void sched_step(void) {
    sched->step();
#ifdef LLAMA_USE_CHCORE_API
    usys_yield();
#endif
}

extern std::atomic<int64_t> decrypt_time;
extern std::atomic<size_t> decrypt_size;
extern std::atomic<int64_t> cma_time;
extern std::atomic<size_t> cma_size;
extern std::atomic<int64_t> io_time;
extern std::atomic<size_t> io_size;

void clear_measure(void) {
    decrypt_time = 0;
    decrypt_size = 0;
    cma_time = 0;
    cma_size = 0;
    io_time = 0;
    io_size = 0;
    use_wait_time = 0;
    use_wait_cpu_time = 0;
}

void dump_measure(void) {
    printf("decrypt time %d ms\n", decrypt_time / 1000);
    printf("decrypt size %d MB\n", decrypt_size / 1024 / 1024);
    printf("cma time %d ms\n", cma_time / 1000);
    printf("cma size %d MB\n", cma_size / 1024 / 1024);
    printf("io time %d ms\n", io_time / 1000);
    printf("io size %d MB\n", io_size / 1024 / 1024);
    printf("use wait io time %d ms\n", (use_wait_time - use_wait_cpu_time) / 1000);
    printf("use wait cpu time %d ms\n", use_wait_cpu_time / 1000);
}

size_t all = 0;

void record_tensor_size(size_t size) {
    all += size;
}

static int cache_p = 0;

void set_cache_proportion(int p) {
    printf("%s to %d\n", __func__, p);
    cache_p = p;
}

void register_param_tensor(
    ggml_tensor *tensor,
    size_t off,
    size_t len,
    int fd
) {
    extern pid_t main_tid;
    main_tid = gettid();
    extern bool is_strawman;
    if (!is_strawman) {
        ggml_set_polling_routine(sched_step);
    }
    if (tensor == NULL) return;
    GGML_ASSERT(param_tensors.find(tensor) == param_tensors.end());

    int layer = parse_name(tensor->name).second;
    static int cnt = 0;
    auto pipeline = std::make_shared<Pipeline>(
        std::make_shared<AllocStage>(off, len),
        std::make_shared<IOStage>(off, len),
        std::make_shared<DecryptStage>(len),
        (void *)((int64_t)layer << 32 | (cnt++))
    );
    // printf("%s %d: %s %p\n", __func__, __LINE__, tensor->name, pipeline->get_sched_info());
    pipeline->set_self();
    auto desc = std::make_shared<param_tensor_desc>(tensor, pipeline);
    param_tensors.emplace(tensor, desc);

    pipeline->get_current_stage()->start(NULL);
    sched->enqueue(pipeline);
    static size_t used = 0;
    used += len;
    if (used <= all * cache_p / 5) {
        printf("use %lu MB of %lu MB\n", used / 1024 / 1024, all / 1024 / 1024);
        use_param_tensor(tensor, 0);
    }
    clear_measure();
    base_time = get_micro();
}

struct ggml_tensor * ggml_use_param_wrapper(
        struct ggml_context * ctx,
        struct ggml_tensor  * self,
        struct ggml_tensor  * dep
) {
    // Tensors loaded via the standard mmap path (use_mmap=true) never go
    // through register_param_tensor(), so they're absent from param_tensors.
    // Wrapping them in USE_PARAM anyway makes use_param_tensor() assert
    // (desc_iter != param_tensors.end()) at inference time. Skip the
    // wrapper for those tensors and pass through the already-correct
    // mmap'd data pointer untouched.
    if (param_tensors.find(self) == param_tensors.end()) {
        return self;
    }
    auto result = ggml_use_param(ctx, self, dep);
    if (result)
        result->extra2 = (void *)use_param_tensor;
    return result;
}