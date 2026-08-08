#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <thread>
#include <mutex>
#include "interface.h"

static int tzd_fd;
static int shm_fd;
static std::once_flag once_flag;
static all_ring_buffer *task_queues;

#define DEVICE_NAME "/dev/tc_ns_client"

void ca_backend_init(const char *io_model_path) {
    tzd_fd = open(DEVICE_NAME, O_RDWR);
    GGML_ASSERT(tzd_fd >= 0);
    void *addr = mmap(NULL, CMD_QUEUE_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, tzd_fd, 0);
    GGML_ASSERT(addr != MAP_FAILED);
    task_queues = (struct all_ring_buffer *)addr;
    task_queues->init();
    extern void io_init(const char *model_path);
    io_init(io_model_path);
}

void ca_backend_io_step(void) {
    GGML_ASSERT(task_queues);
    extern void io_step(all_ring_buffer *task_queues);
    io_step(task_queues);
}

void ca_backend_set_cache(int p) {
    GGML_ASSERT(task_queues);
    sprintf(task_queues->cache_p, "%d\0", p);
}

void ca_backend_set_prompt(const char *model, int len) {
    GGML_ASSERT(task_queues);
    sprintf(task_queues->prompt, "%s#%d\0", model, len);
    if (!strcmp(model, "tinyllama")) {
        strcpy(task_queues->inner_model_path, "tinyllama-1.1b-chat-v1.0.Q8_0-meta.gguf");
    } else if (!strcmp(model, "gemma")) {
        strcpy(task_queues->inner_model_path, "gemma-2-2b-it-Q8_0-meta.gguf");
    } else if (!strcmp(model, "qwen")) {
        strcpy(task_queues->inner_model_path, "qwen2.5-3b-instruct-q8_0-meta.gguf");
    } else if (!strcmp(model, "phi")) {
        strcpy(task_queues->inner_model_path, "Phi-3-mini-4k-instruct.Q8_0-meta.gguf");
    } else if (!strcmp(model, "llama")) {
        strcpy(task_queues->inner_model_path, "Meta-Llama-3-8B-Instruct.Q8_0-meta.gguf");
    } else {
        GGML_ABORT("model: %s\n", model);
    }
}

void ca_backend_set_prompt_text(const char *model, const char *text) {
    GGML_ASSERT(task_queues);
    snprintf(task_queues->prompt, sizeof(task_queues->prompt), "%s#%s", model, text);
    if (!strcmp(model, "tinyllama")) {
        strcpy(task_queues->inner_model_path, "tinyllama-1.1b-chat-v1.0.Q8_0-meta.gguf");
    } else if (!strcmp(model, "gemma")) {
        strcpy(task_queues->inner_model_path, "gemma-2-2b-it-Q8_0-meta.gguf");
    } else if (!strcmp(model, "qwen")) {
        strcpy(task_queues->inner_model_path, "qwen2.5-3b-instruct-q8_0-meta.gguf");
    } else if (!strcmp(model, "phi")) {
        strcpy(task_queues->inner_model_path, "Phi-3-mini-4k-instruct.Q8_0-meta.gguf");
    } else if (!strcmp(model, "llama")) {
        strcpy(task_queues->inner_model_path, "Meta-Llama-3-8B-Instruct.Q8_0-meta.gguf");
    } else {
        GGML_ABORT("model: %s\n", model);
    }
}

void ca_backend_set_n(int n) {
    GGML_ASSERT(task_queues);
    sprintf(task_queues->n, "%d\0", n);
}

void ca_backend_set_strawman(bool is_strawman) {
    GGML_ASSERT(task_queues);
    task_queues->is_strawman = is_strawman;
}

// Real result channel: the TA (main.cpp) publishes the final generated
// answer into this same shared command-queue page once inference
// completes (see FINAL_ANSWER_MAX's comment, interface.h) -- this is the
// only reliable way to get it out, since the TA's UART console output is
// shared with every other concurrently-printing thread/kernel subsystem
// and has repeatedly corrupted this exact text. Returns true exactly once
// (the first call after the answer becomes available) so callers can poll
// this from a busy loop without printing duplicates.
bool ca_backend_poll_final_answer(char *out, size_t out_size) {
    GGML_ASSERT(task_queues);
    bool expected = true;
    if (!task_queues->final_answer_ready.compare_exchange_strong(expected, false)) {
        return false;
    }
    snprintf(out, out_size, "%s", task_queues->final_answer);
    return true;
}

// Diagnostic: same reasoning as ca_backend_poll_final_answer -- relays
// main.cpp's [LOGIT_DIAG] top-5 out of the secure world via the shared
// page (see interface.h's logit_diag_* comment). Returns the current
// n_past (-1 if none published yet); caller compares against its own
// last-seen value to detect a new one (mirrors mul_mat_progress's pattern
// from earlier this session, minus that field, which isn't in this build).
int ca_backend_poll_logit_diag(int *top_idx, float *top_val) {
    GGML_ASSERT(task_queues);
    int n_past = task_queues->logit_diag_n_past.load();
    memcpy(top_idx, task_queues->logit_diag_top_idx, sizeof(int) * 5);
    memcpy(top_val, task_queues->logit_diag_top_val, sizeof(float) * 5);
    return n_past;
}

void ca_backend_submit_request(const char *model, const char *text, int n, int cache, bool is_strawman) {
    GGML_ASSERT(task_queues);
    ca_backend_set_cache(cache);
    ca_backend_set_prompt_text(model, text);
    ca_backend_set_n(n);
    ca_backend_set_strawman(is_strawman);
    task_queues->final_answer_ready.store(false);
    task_queues->request_ready.store(true);
}

