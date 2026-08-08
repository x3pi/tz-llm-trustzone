#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ggml.h"
#include <sys/ioctl.h>
#include <cstring>
#include <pthread.h>
#include <cstdlib>
#include <thread>
#include <vector>
#include <getopt.h>
#include <string>
#include <sched.h>
#include <csignal>
#include <atomic>
#include <chrono>
#include <ctime>
#include "interface.h"
#include "../server/httplib.h"
#include "../../common/json.hpp"

using json = nlohmann::json;

extern void ca_backend_submit_request(const char *model, const char *text, int n, int cache, bool is_strawman);
std::atomic<int> ca_state(0); // 0=IDLE, 1=BUSY

extern void dbg_log_dump(void);

// Trigger point for diagnosing a hang externally without perturbing the
// hot-path timing: `kill -USR1 <pid>` from another shell dumps the last
// DBG_LOG_SIZE io-backend events (io_step/wait_io/get_buf) seen so far,
// showing whether CA-side ever reached userspace IO handling at all or is
// still spinning entirely inside the kernel SMC relay loop.
static void sigusr1_dump_handler(int) {
    dbg_log_dump();
}

struct llm_client_op_pages {
	unsigned long entry_begin;
	unsigned long entry_end;
};

#define DEVICE_NAME "/dev/tc_ns_client"
#define TC_NS_CLIENT_IOC_MAGIC  't'
#define LLM_CLIENT_IOCTL_RUN \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 24, int)
#define LLM_CLIENT_IOCTL_ALLOC_PAGES \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 25, struct llm_client_op_pages)
#define LLM_CLIENT_IOCTL_FREE_PAGES \
	_IOWR(TC_NS_CLIENT_IOC_MAGIC, 26, struct llm_client_op_pages)

enum smc_loop_exit {
    SMC_LOOP_EXIT_FINISH = 1,
    SMC_LOOP_EXIT_NPU_SUBMIT,
    SMC_LOOP_EXIT_NPU_DONE,
    SMC_LOOP_EXIT_IO_STEP,
};

extern void ca_backend_init(const char *io_model_path);
extern void ca_backend_io_step(void);
extern void ca_backend_set_cache(int p);
extern void ca_backend_set_prompt(const char *model, int len);
extern void ca_backend_set_prompt_text(const char *model, const char *text);
extern void ca_backend_set_n(int n);
extern void ca_backend_set_strawman(bool is_strawman);
extern bool ca_backend_poll_final_answer(char *out, size_t out_size);
extern int ca_backend_poll_logit_diag(int *top_idx, float *top_val);

void ca_thread(int fd, int index) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4 + index, &cpuset);
    GGML_ASSERT(pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);

    printf("%s %d run llm\n", __func__, __LINE__);
    while (true) {
        int out_cmd;
        unsigned long ret = ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd, &out_cmd);
        // printf("%s %d: ret %d error %d\n", __func__, __LINE__, ret, errno);
        // GGML_ASSERT(ret >= 0);
        // switch (out_cmd) {
        // case SMC_LOOP_EXIT_IO_STEP:
        //     printf("%s %d: receive SMC_LOOP_EXIT_IO_STEP\n", __func__, __LINE__);
            ca_backend_io_step();
        //     break;
        // default:
        //     GGML_ASSERT(false);
        //     break;
        // }
        // PERFORMANCE: this relay loop runs at very high frequency (only
        // a sched_yield() between iterations) and this atomic check
        // touches a cache line in TA-shared secure-world memory --
        // confirmed via direct measurement to add enough per-iteration
        // cost, done this often, to stall the whole pipeline (a run that
        // normally shows progress within 1-2 minutes produced ZERO
        // progress markers after 12+ minutes with this checked every
        // iteration). Rate-limit to once every ~2000 iterations -- still
        // sub-second latency for the answer to actually get printed once
        // ready, at a small fraction of the original polling rate.
        if (index == 0) {
            static int poll_ctr = 0;
            if (++poll_ctr >= 2000) {
                poll_ctr = 0;
                static int last_logit_n_past = -1;
                int top_idx[5];
                float top_val[5];
                int n_past = ca_backend_poll_logit_diag(top_idx, top_val);
                if (n_past != last_logit_n_past) {
                    last_logit_n_past = n_past;
                    printf("[SECURE_LOGIT_DIAG] n_past=%d top5=[%d:%.4f,%d:%.4f,%d:%.4f,%d:%.4f,%d:%.4f]\n",
                        n_past,
                        top_idx[0], top_val[0], top_idx[1], top_val[1],
                        top_idx[2], top_val[2], top_idx[3], top_val[3],
                        top_idx[4], top_val[4]);
                    fflush(stdout);
                }
            }
        }
        // Yield after each SMC round-trip so this busy-poll loop (pinned to
        // dedicated cores) doesn't monopolize the CPU enough to starve
        // unrelated kernel work (RCU grace periods, WiFi RX softirq, etc.)
        // on this board/kernel - was causing repeated soft lockups.
        sched_yield();
    }
}
int main(int argc, char *argv[]) {
    int fd;
    char *mapped_mem;
    size_t length = getpagesize();

    // stdout defaults to fully-buffered when not attached to a real TTY
    // (e.g. run over a UART shell / hdc) - printf output was silently
    // sitting in libc's buffer indefinitely instead of ever appearing,
    // which made every debugging attempt this session blind to progress.
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGUSR1, sigusr1_dump_handler);

    fd = open(DEVICE_NAME, O_RDWR);
    GGML_ASSERT(fd > 0);

    mapped_mem = (char *)mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    GGML_ASSERT(mapped_mem != MAP_FAILED);

    printf("msg in shm %s\n", (char *)mapped_mem);

    const char *model = "qwen";
    int cache = 0;
    int len = 128;
    int n = 64;
    int strawman = 0;
    const char *text = NULL;

    struct option long_options[] = {
        {"model", required_argument, NULL, 'm'},
        {"cache", required_argument, NULL, 'c'},
        {"len", required_argument, NULL, 'l'},
        {"nn", required_argument, NULL, 'n'},
        {"strawman", required_argument, NULL, 's'},
        {"text", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "m:c:l:n:s:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                model = optarg;
                break;
            case 'c':
                cache = atoi(optarg);
                break;
            case 'l':
                len = atoi(optarg);
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 's':
                strawman = atoi(optarg);
                printf("strawman %d\n", strawman);
                break;
            case 't':
                // Literal free-text prompt (e.g. "hello"), bypassing the
                // fixed-length benchmark prompt table used by -l/--len.
                text = optarg;
                break;
            case '?':
                // Invalid argument or missing value
                break;
        }
    }

    std::string model_string(model);
    const char *io_model_path;
    if (model_string == "tinyllama") {
        io_model_path = "/data/ssd/tinyllama-1.1b-chat-v1.0.Q8_0.gguf";
    } else if (model_string == "gemma") {
        io_model_path = "/data/ssd/gemma-2-2b-it-Q8_0.gguf";
    } else if (model_string == "qwen") {
        io_model_path = "/data/ssd/qwen2.5-3b-instruct-q8_0.gguf";
    } else if (model_string == "phi") {
        io_model_path = "/data/ssd/Phi-3-mini-4k-instruct.Q8_0.gguf";
    } else if (model_string == "llama") {
        io_model_path = "/data/ssd/Meta-Llama-3-8B-Instruct.Q8_0.gguf";
    } else {
        GGML_ABORT("invalid model %s\n", model);
    }

    ca_backend_init(io_model_path);
    ca_backend_set_cache(cache);
    if (text != NULL) {
        ca_backend_set_prompt_text(model, text);
    } else {
        ca_backend_set_prompt(model, len);
    }
    ca_backend_set_n(n);
    ca_backend_set_strawman((bool)strawman);

    std::vector<std::thread> ca_threads;
    for (int i = 0; i < 4; i++) {
        ca_threads.emplace_back(ca_thread, fd, i);
    }
    for (int i = 0; i < 4; i++) {
        ca_threads[i].detach();
    }

    printf("Waiting for warmup request to complete...\n");
    char warmup_answer[FINAL_ANSWER_MAX];
    while (!ca_backend_poll_final_answer(warmup_answer, sizeof(warmup_answer))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    printf("Warmup complete.\n");

    httplib::Server svr;
    svr.Post("/completion", [model, n, cache, strawman](const httplib::Request &req, httplib::Response &res) {
        if (ca_state.exchange(1) != 0) {
            res.status = 503;
            res.set_content("{\"error\":\"TA is busy\"}", "application/json");
            return;
        }

        std::string prompt_text;
        try {
            json j = json::parse(req.body);
            if (j.contains("prompt")) {
                prompt_text = j["prompt"].get<std::string>();
            } else {
                ca_state.store(0);
                res.status = 400;
                res.set_content("{\"error\":\"Missing prompt\"}", "application/json");
                return;
            }
        } catch (...) {
            ca_state.store(0);
            res.status = 400;
            res.set_content("{\"error\":\"Invalid JSON\"}", "application/json");
            return;
        }

        ca_backend_submit_request(model, prompt_text.c_str(), n, cache, strawman);

        auto start = std::chrono::steady_clock::now();
        char answer[FINAL_ANSWER_MAX];
        bool answered = false;
        
        while (true) {
            if (ca_backend_poll_final_answer(answer, sizeof(answer))) {
                answered = true;
                break;
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 300) {
                // Watchdog trigger: request timed out, assume the TA is
                // wedged (same failure class as the historical probabilistic
                // hang, not yet root-caused -- see DEPLOYED_STATE.md). Log
                // via direct file I/O, not system("echo ...."), since
                // prompt_text is untrusted request input and must never be
                // interpolated into a shell command string.
                FILE *wf = fopen("/data/ssd/watchdog.log", "a");
                if (wf) {
                    time_t now_t = time(nullptr);
                    char ts[32];
                    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now_t));
                    fprintf(wf, "[%s] watchdog timeout after 300s, rebooting. prompt=%s\n",
                        ts, prompt_text.c_str());
                    fclose(wf);
                }
                system("reboot");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (answered) {
            json resp;
            resp["content"] = answer;
            res.set_content(resp.dump(), "application/json");
        } else {
            res.status = 504;
            res.set_content("{\"error\":\"Timeout\"}", "application/json");
        }
        
        ca_state.store(0);
    });

    printf("Starting HTTP server on 0.0.0.0:8080\n");
    svr.listen("0.0.0.0", 8080);

#define LLM_MODEL_SIZE (4ul * 1024 * 1024 * 1024)
    struct llm_client_op_pages op = { .entry_begin = 0, .entry_end = LLM_MODEL_SIZE / (1 << 12) };
    printf("%s %d\n", __func__, __LINE__);
    GGML_ASSERT(ioctl(fd, LLM_CLIENT_IOCTL_ALLOC_PAGES, &op) >= 0);
    printf("%s %d\n", __func__, __LINE__);
    GGML_ASSERT(ioctl(fd, LLM_CLIENT_IOCTL_RUN, fd) >= 0);
    printf("%s %d\n", __func__, __LINE__);
    GGML_ASSERT(ioctl(fd, LLM_CLIENT_IOCTL_FREE_PAGES, &op) >= 0);
    printf("%s %d\n", __func__, __LINE__);

    close(fd);

    return 0;
}
