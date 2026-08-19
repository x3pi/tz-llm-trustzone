/*
 * mvm_launcher: metanode's OWN, fully independent process launcher for
 * /mvm_ta (2026-08-19, GĐ3 — see metanode/note/tee_dual_mode_execution_plan.md
 * §9.22-9.23). Deliberately a completely separate binary/build target from
 * chanmgr.srv, launched independently by procmgr's own boot_default_apps()
 * — not a call site inside chanmgr/main.c. This is the explicit fix for a
 * hardware-confirmed hazard: chanmgr.srv also launches llama-cli, whose own
 * CA-side test/relay tooling (fake_ca.cpp's ca_thread) and metanode's own
 * CA-side relay (mvm_ca_test.cpp) both call the SAME shared
 * LLM_CLIENT_IOCTL_RUN ioctl (single-slot-per-CPU usys_tee_wait_switch_req()
 * rendezvous in the kernel) — running both processes' relays concurrently
 * crashed llama-cli outright (kernel BUG_ON when both a metanode-side and
 * llama-cli-side wait landed on the same CPU). Splitting the LAUNCH itself
 * into a separate binary means a metanode-only deployment can boot without
 * ever starting chanmgr.srv/llama-cli at all — see procmgr.c's own
 * boot_default_apps() for the corresponding launch-gating change.
 *
 * Scope is deliberately minimal: reproduce ONLY the two things mvm_ta
 * actually depends on from chanmgr's own main()) -- the 16 priority-1 idle
 * threads that are what actually hands CPU time back to Normal World (see
 * mvm_ta_main.cpp's own mvm_wait_for_boot_settled() comment for the full
 * mechanism) -- and launch + supervise /mvm_ta. No channel/IPC dispatch
 * logic, no llama-cli awareness, no shared struct with chanmgr.c at all.
 */
#include <chcore/syscall.h>
#include <chcore/launcher.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/wait.h>

/*
 * Copied verbatim from chanmgr/main.c's own idle() -- this exact pattern
 * (drop to priority 1, then loop calling the real blocking
 * usys_tee_switch_req()) is what actually lets Normal World get scheduled
 * at all; see [[mvm-ta-normal-world-priority-starvation]] memory /
 * mvm_ta_main.cpp's own mvm_wait_for_boot_settled() comment for the full
 * incident this was root-caused from. Duplicated here (not linked/shared)
 * per the explicit project-separation requirement -- it's a small, proven,
 * self-contained idiom, not shared state.
 */
static void *idle(void *arg)
{
    (void)arg;
    usys_disable_local_irq();
    usys_set_prio(0, 1);
    usys_yield();
    while (1) {
        struct smc_registers regs = {0};
        usys_tee_switch_req(&regs);
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("%s %d: mvm_launcher main entry\n", __func__, __LINE__);

    for (int i = 0; i < 16; i++) {
        pthread_t t;
        if (pthread_create(&t, NULL, idle, NULL) != 0) {
            perror("mvm_launcher: failed to create idle thread");
            return 1;
        }
        usys_yield(); usys_yield(); usys_yield(); usys_yield(); usys_yield(); usys_yield();
    }

    char *mvm_argv[] = { "/mvm_ta" };
    pid_t pid = create_process(1, mvm_argv, NULL);
    if (pid < 0) {
        printf("%s %d: FATAL: failed to launch /mvm_ta (ret=%d)\n",
               __func__, __LINE__, pid);
        return 1;
    }
    printf("%s %d: launched metanode TA, pid=%d\n", __func__, __LINE__, pid);

    int ret = waitpid(pid, NULL, 0);
    printf("mvm_launcher: metanode TA (pid=%d) exited, waitpid ret=%d -- "
           "this TA runs forever under normal operation, reaching here "
           "means it crashed or exited early, not expected\n", pid, ret);
    return 0;
}
