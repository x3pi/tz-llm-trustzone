/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore-internal/chanmgr_defs.h>
#include <errno.h>
#include <chanmgr.h>
#include <pthread.h>
#include <chcore/memory.h>
#include <string.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <chcore/defs.h>
#include <chcore/llm.h>

void chanmgr_dispatch(ipc_msg_t *ipc_msg, badge_t client_badge, int pid,
                      int tid)
{
    struct chan_request *req;

    req = (struct chan_request *)ipc_get_msg_data(ipc_msg);

    switch (req->req) {
    case CHAN_REQ_CREATE_CHANNEL:
        chanmgr_handle_create_channel(ipc_msg, client_badge, pid, tid);
        break;
    case CHAN_REQ_REMOVE_CHANNEL:
        chanmgr_handle_remove_channel(ipc_msg, client_badge, pid, tid);
        break;
    case CHAN_REQ_HUNT_BY_NAME:
        chanmgr_handle_hunt_by_name(ipc_msg, pid, tid);
        break;
    case CHAN_REQ_GET_CH_FROM_PATH:
        chanmgr_handle_get_ch_from_path(ipc_msg, pid, tid);
        break;
    case CHAN_REQ_GET_CH_FROM_TASKID:
        chanmgr_handle_get_ch_from_taskid(ipc_msg, pid, tid);
        break;
    default:
        ipc_return(ipc_msg, -EBADRQC);
        break;
    }
}

#define NUM_THREADS 10

pthread_mutex_t mutex;
pthread_cond_t cond_var;

void* worker(void* arg) {
    int thread_id = *(int*)arg;
    free(arg);

    int cnt = 0;
    while (true) {
        pthread_mutex_lock(&mutex);
        printf("Thread %d is waiting.\n", thread_id);
        pthread_cond_wait(&cond_var, &mutex);
        printf("Thread %d is awakened counter %d.\n", thread_id, cnt);
        pthread_mutex_unlock(&mutex);

        while (true) {
            int sum = 0;
            for (int i = 0; i < 1000000000; i++) {
                sum += (i ^ 123154231);
            }
            printf("sum %d\n", sum);
        }
    }

    return NULL;
}

void *idle(void *arg) {
    usys_disable_local_irq();
    usys_set_prio(0, 1);
    usys_yield();
    while (1) {
        struct smc_registers regs = {0};
        usys_tee_switch_req(&regs);
    }
}

void *loop(void *) {
    while (1);
}

int master() {
    printf("%s %d: master() entry\n", __func__, __LINE__);
    pthread_t threads[NUM_THREADS];
    struct smc_registers req = {0};

    pthread_t loop_thread;

    printf("SHM INIT: Main thread is waiting for smc\n");
    unsigned long paddr = usys_tee_wait_switch_req(&req);
    printf("SHM INIT: Main thread is awaken from smc\n");
    unsigned long size = CMD_QUEUE_SHM_SIZE;
    printf("received shm addr %p size %lx\n", (void *)paddr, size);
    cap_t pmo = usys_tee_create_ns_pmo(paddr, size);
    void *vaddr = chcore_auto_map_pmo(pmo, size, VMR_READ | VMR_WRITE);

    sprintf((char *)vaddr, "msg from tee\n");

    if (0) {
        printf("Main thread is waiting for smc\n");
        usys_tee_wait_switch_req(&req);
        printf("Main thread is awaken from smc\n");

    #define LLM_MODEL_SIZE (4ul * 1024 * 1024 * 1024)
        printf("%s %d\n", __func__, __LINE__);
        cap_t pmo = usys_create_s2_pmo(0, LLM_MODEL_SIZE / PAGE_SIZE);
        printf("%s %d\n", __func__, __LINE__);
        void *buf = chcore_auto_map_pmo(pmo, LLM_MODEL_SIZE, VMR_READ | VMR_WRITE);
        printf("%s %d\n", __func__, __LINE__);
        memset(buf, 0, LLM_MODEL_SIZE);
        printf("%s %d\n", __func__, __LINE__);
        chcore_auto_unmap_pmo(pmo, (unsigned long)buf, LLM_MODEL_SIZE);
        printf("%s %d\n", __func__, __LINE__);

        printf("Main thread is waiting for smc\n");
        usys_tee_wait_switch_req(&req);
        printf("Main thread is awaken from smc\n");
    }

    // Initialize mutex and condition variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_var, NULL);

    // Create worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int* thread_id = malloc(sizeof(int));
        *thread_id = i;
        if (pthread_create(&threads[i], NULL, worker, thread_id) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS * 100; i++) usys_yield();

    printf("Main thread is waiting for smc\n");
    usys_tee_wait_switch_req(&req);
    printf("Main thread is awaken from smc\n");

    if (0) {
        int ret;
        vaddr_t vaddr;
        vaddr = chcore_alloc_vaddr(PAGE_SIZE << 10);
        BUG_ON(vaddr == 0);
        ret = usys_map_tzasc_cma_meta(vaddr);
        BUG_ON(ret != 0);
        struct tzasc_cma_meta *tzasc_cma_meta = (struct tzasc_cma_meta *)vaddr;
        printf("%s %d base %#lx size %#lx\n", __func__, __LINE__, tzasc_cma_meta->base, tzasc_cma_meta->size);

        req.x1 = SMC_EXIT_SHADOW;
        req.x2 = 1;
        req.x3 = 16UL << 20;
        ret = usys_tee_switch_req(&req);
        printf("%s %d ret %d count %#lx\n", __func__, __LINE__, ret, tzasc_cma_meta->count);

        req.x1 = SMC_EXIT_SHADOW;
        req.x2 = 0;
        ret = usys_tee_switch_req(&req);
        printf("%s %d ret %d count %#lx\n", __func__, __LINE__, ret, tzasc_cma_meta->count);
        req.x1 = SMC_EXIT_SHADOW;
        req.x2 = 0;
        ret = usys_tee_switch_req(&req);
        printf("%s %d ret %d count %#lx\n", __func__, __LINE__, ret, tzasc_cma_meta->count);

        while (1);
    }

    pthread_create(&loop_thread, NULL, loop, NULL);
    printf("chanmgr: create rknpu\n");
    char *rknpu_argv[] = { "/rknpu.srv", "32", "32", "32" };
    pid_t pid = create_process(4, &rknpu_argv, NULL);
    int ret = waitpid(pid, NULL, 0);
    printf("chanmgr: rknpu test finish\n");

    // Wake up all threads
    pthread_mutex_lock(&mutex);
    printf("Main thread broadcasting to all workers.\n");
    pthread_cond_broadcast(&cond_var);
    pthread_mutex_unlock(&mutex);

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy mutex and condition variable
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_var);

    BUG_ON(1);

    return 0;
}


struct all_ring_buffer_header {
    char io_model_path[256];
    char cache_p[256];
    char inner_model_path[256];
    char prompt[256];
    char n[256];
};

/*
 * metanode dual-mode-execution TA (2026-08-17, GĐ3 — see
 * metanode/note/tee_dual_mode_execution_plan.md §9). Deliberately a
 * completely separate process/binary from llama-cli — no shared code, no
 * shared struct, no shared build target (explicit project-separation
 * requirement). Its own execution/pkg/mvm/ta/mvm_ta_main.cpp reserves its
 * own CA<->TA shared channel via push_pages_ex()+usys_map_tzasc_cma_pmo,
 * the same primitives this repo's own alloc-stage-chcore.cpp uses for
 * model-weight streaming — reimplemented standalone there, not linked
 * from this repo.
 *
 * waitpid() on it is pushed into its own thread (this TA runs forever
 * under normal operation, same as llama-cli) so it doesn't stall main()
 * from reaching the llama-cli launch right after — see main()'s own call
 * site below for why the create_process() call itself must NOT be moved
 * into this thread (launch-order/entry_index=0 determinism, plan §9.5).
 *
 * UNVERIFIED as of 2026-08-17: this file could not be compiled in this
 * session (needs the full chanmgr/chcore build environment, not just the
 * userspace TA toolchain already verified separately) — first real build
 * attempt will surface any syntax/API mismatch here.
 */
static pid_t g_mvm_ta_pid = -1;

static void *mvm_ta_waiter(void *arg)
{
    (void)arg;
    int ret = waitpid(g_mvm_ta_pid, NULL, 0);
    printf("chanmgr: metanode TA (pid=%d) exited, waitpid ret=%d — this TA "
           "runs forever under normal operation, reaching here means it "
           "crashed or exited early, not expected\n", g_mvm_ta_pid, ret);
    return NULL;
}

int main(void)
{
    int ret;

    /*
     * Unbuffered: this is a long-lived process whose stdout is fully buffered
     * by default, so lines reach the UART out of order (or not at all). That
     * made the boot log unusable for reasoning about execution order -- e.g.
     * master()'s "SHM INIT" appeared after the llama-cli block that is supposed
     * to precede it, which cannot happen from a plain reading of the source.
     */
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("%s %d: chanmgr main entry\n", __func__, __LINE__);

    for (int i = 0; i < 16; i++) {
        pthread_t t;
        if (pthread_create(&t, NULL, idle, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        usys_yield(); usys_yield(); usys_yield(); usys_yield(); usys_yield(); usys_yield();
    }

    if (0) {

        printf("%s %d\n", __func__, __LINE__);
        usys_config_tzasc(8, 0x100000000UL >> 20, 0x140000000UL >> 20);
        usys_config_tzasc(8, 0x140000000UL >> 20, 0x140000000UL >> 20);
        printf("%s %d\n", __func__, __LINE__);

        // return 0;
    }

    /*
     * llama-cli must launch here, directly from main(), ending in `return 0`
     * before master() ever runs. Two earlier attempts this session got this
     * wrong in opposite directions:
     *
     * 1) Launching /rknpu.srv from main() (before Linux/REE exist): it issued
     *    a real fp16 matmul then blocked in waitpid() forever, since the
     *    TEE-side NPU driver is data-plane only (paper S4.3) and needs the
     *    REE control plane, which does not exist yet.
     *
     * 2) Moving this llama-cli launch into master(), after its "rknpu test
     *    finish" (so Linux/REE existed and rknpu.srv's self-test had already
     *    run cleanly): llama-cli's own SHM handshake in main.cpp
     *    (usys_tee_wait_switch_req(), twice, to receive `all_ring_buffer
     *    *task_queue` from the CA) then received `paddr = 0` and the TEE
     *    kernel crashed (`BUG: unexpected_handler`, IP: 0), which cascaded
     *    into a full Linux kernel panic ~90s later.
     *
     *    Root cause: usys_tee_wait_switch_req() is a shared, FIFO-like
     *    rendezvous -- whichever TEE thread is blocked in it next consumes
     *    the next SMC that arrives from the REE, regardless of who it was
     *    "meant" for. master()'s own SHM INIT + its second wait-for-smc (both
     *    ahead of "chanmgr: create rknpu" in the source, unrelated to
     *    llama-cli -- see the worker-thread/TZASC scaffolding below) eat two
     *    SMC slots before llama-cli's handshake gets a turn, so llama-cli
     *    ends up consuming whatever (empty/stale) SMC comes after, not the
     *    CA's real handshake payload.
     *
     *    master() is a separate, self-contained test harness (worker thread
     *    stress test, commented-out TZASC/CMA experiments) that upstream
     *    never intended to run in the same process as the real llama-cli
     *    launch -- that's why upstream's own `if (1)` here `return 0`s before
     *    master() is ever reached. Left disabled below on purpose.
     *
     * Actual NPU job submission during inference does NOT go through
     * /rknpu.srv or master() at all: ggml's rknpu backend links the same
     * rknpu-driver.c and calls usys_tee_switch_req(SMC_EXIT_SHADOW) directly,
     * serviced by tzdriver's own FIQ-triggered shadow-job kernel worker
     * (smc_smp.c: fiq_shadow_work_func/smc_queue_shadow_worker) -- no
     * userspace REE process needs to be running for that to work.
     */

    /*
     * metanode's TA MUST launch here, before llama-cli below — see
     * mvm_ta_waiter's doc comment above. create_process() itself runs
     * synchronously right here (not deferred to a thread) so this TA is
     * guaranteed the temporally-first process to touch TZASC memory on
     * this boot, which its own push_pages() reservation depends on for a
     * deterministic entry_index=0 (metanode/note/
     * tee_dual_mode_execution_plan.md §9.5). A launch failure here is
     * logged but NOT fatal to chanmgr — the llama-cli path below is
     * completely unaffected either way, matching the project-separation
     * requirement this whole mechanism exists to satisfy.
     */
    {
        char *mvm_argv[] = { "/mvm_ta" };
        g_mvm_ta_pid = create_process(1, (char **)mvm_argv, NULL);
        if (g_mvm_ta_pid < 0) {
            printf("%s %d: WARNING: failed to launch metanode TA (ret=%d) -- "
                   "continuing without it, llama-cli path unaffected\n",
                   __func__, __LINE__, g_mvm_ta_pid);
        } else {
            pthread_t mvm_waiter_thread;
            pthread_create(&mvm_waiter_thread, NULL, mvm_ta_waiter, NULL);
            printf("%s %d: launched metanode TA, pid=%d\n",
                   __func__, __LINE__, g_mvm_ta_pid);
        }
    }

    if (1) {
        const char *argv[] = {
            /*
             * tinyllama instead of Meta-Llama-3-8B: the 8B meta gguf plus its
             * weights do not fit the 768MiB TZASC pool on this board, and its
             * -meta.gguf was never baked into oh_tee/apps anyway. The '#0'
             * suffix selects UltraChat prompt 0 (see parse_prompt() in
             * common/arg.cpp); the part before '#' picks the full gguf that
             * gets streamed in from /data/ssd on the REE side.
             */
            "llama-cli",
            "-m", "tinyllama-1.1b-chat-v1.0.Q8_0-meta.gguf",
            "--no-warmup",
            "-p", "tinyllama#Hello, how are you today?",
            "--cache", "0",
            "-n", "64",
            "-s", "123",
            "-ngl", "100",
            "-t", "4",
            "-c", "1124",
            "--no-mmap"
        };
        char argc = sizeof(argv) / sizeof(*argv);
        printf("%s %d: launching llama\n", __func__, __LINE__);
        pid_t pid = create_process(argc, (char **)argv, NULL);
        int ret = waitpid(pid, NULL, 0);

        printf("%s %d: llama.cpp finished\n", __func__, __LINE__);
        {
            
            struct smc_registers req = {0};
            req.x1 = SMC_EXIT_SHADOW;
            req.x2 = 0xdeadbeef;
            int ret = usys_tee_switch_req(&req);
            BUG_ON(ret);
        }
        usys_top(0);
        while (1);

        return 0;
    }

    printf("%s %d: falling through to master()\n", __func__, __LINE__);
    master();

    return 0;

    chanmgr_init();

    ret = ipc_register_server_with_destructor(
        chanmgr_dispatch, DEFAULT_CLIENT_REGISTER_HANDLER, chanmgr_destructor);
    printf("[chanmgr] register server value = %d\n", ret);

    usys_exit(0);
}
