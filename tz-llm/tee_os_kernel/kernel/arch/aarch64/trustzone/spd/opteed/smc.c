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
#include <mm/mm.h>
#include <machine.h>
#include <sched/sched.h>
#include <object/thread.h>
#include <mm/uaccess.h>
#include <arch/machine/smp.h>
#include <arch/trustzone/smc.h>
#include <arch/trustzone/tlogger.h>

struct lock smc_struct_lock;
struct lock tzasc_cma_meta_init_lock;
struct smc_percpu_struct {
    struct thread *waiting_thread;
    struct smc_registers regs_k;
    struct smc_registers *regs_u;
} smc_percpu_structs;

paddr_t smc_ttbr0_el1;

#define SMC_ASID 1000UL
static void init_smc_page_table(void)
{
    extern ptp_t boot_ttbr0_l0;

    /* Reuse the boot stage low memory page table */
    smc_ttbr0_el1 = (paddr_t)&boot_ttbr0_l0;
    smc_ttbr0_el1 |= SMC_ASID << 48;
}

void smc_init(void)
{
    u32 cpuid;
    struct smc_percpu_struct *percpu;

    percpu = &smc_percpu_structs;
    percpu->waiting_thread = NULL;

    init_smc_page_table();
}

static bool kernel_shared_var_recved = false;
static kernel_shared_varibles_t kernel_var;

void handle_yield_smc(unsigned long x0, unsigned long x1, unsigned long x2,
                      unsigned long x3, unsigned long x4)
{
    int ret;
    struct smc_percpu_struct *percpu;
    static bool meta_init = false;
    // kinfo("DEBUG: %s CPU %d: x: [%lx %lx %lx %lx %lx]\n",
    //     __func__, smp_get_cpu_id(), x0, x1, x2, x3, x4);

#if FPU_SAVING_MODE == EAGER_FPU_MODE
    save_fpu_state(ree_thread);
#endif

    BUG_ON(current_thread);

    lock(&tzasc_cma_meta_init_lock);
    if (!meta_init) {
        meta_init = true;
        // s2_meta_init(x3);
        tzasc_cma_meta_init(x4);
    }
    unlock(&tzasc_cma_meta_init_lock);

    if (percpu->waiting_thread) {
        percpu = &smc_percpu_structs;
        lock(&smc_struct_lock);
        percpu->regs_k.x0 = TZ_SWITCH_REQ_STD_REQUEST;
        percpu->regs_k.x1 = x1;
        percpu->regs_k.x2 = x2;
        percpu->regs_k.x3 = x3;
        percpu->regs_k.x4 = x4;

        // kinfo("%s %d: wake up waiting thread\n", __func__, __LINE__);
        // kinfo("%s\n", percpu->waiting_thread->cap_group->cap_group_name);
        // switch_vmspace_to(percpu->waiting_thread);
        // BUG_ON(copy_to_user(percpu->regs_u, &percpu->regs_k, sizeof(struct smc_registers)));
        arch_set_thread_return(percpu->waiting_thread, x1);
        percpu->waiting_thread->thread_ctx->state = TS_INTER;
        BUG_ON(sched_enqueue(percpu->waiting_thread));
        percpu->waiting_thread = NULL;
        unlock(&smc_struct_lock);
    }
    // kinfo("%s %d: cpu %d enters tee\n", __func__, __LINE__, smp_get_cpu_id());

    if (x2 >= KBASE) {
        struct thread *thread = (struct thread *)x2;
        arch_set_thread_return(thread, x1);
        BUG_ON(sched_enqueue(thread));
        /* TEMP DIAGNOSTIC: pairs with [TZLLM_TRACE] in tc_client_driver.c's
         * smc_call_cpu_resume() -- confirms this exact thread's push/pop
         * request actually got relayed back into ChCore and woken here,
         * vs staying parked forever (the observed stall symptom: CA-side
         * io_step ring buffer goes silent, no crash, no dmesg error). */
        static int wake_ctr = 0;
        if ((++wake_ctr % 2000) == 1)
            kinfo("[TZLLM_TRACE] handle_yield_smc: waking thread %p (#%d)\n", thread, wake_ctr);
        // kinfo("%s %d wake up thread %p ret %lx\n", __func__, __LINE__, thread, x1);
        extern struct tzasc_cma_meta *tzasc_cma_meta;
        // kinfo("%s %d count %lx\n", __func__, __LINE__, tzasc_cma_meta->count);
#if FPU_SAVING_MODE == LAZY_FPU_MODE
        change_fpu_owner_to_ree();
#endif
        smc_call(SMC_STD_RESPONSE, SMC_EXIT_PREEMPTED);
    }

    sched();
    // kinfo("current thread %s pc %p\n", current_thread, (void *)arch_get_thread_next_ip(current_thread));
    eret_to_thread(switch_context());
}

void handle_fiq_smc(unsigned long x0, unsigned long x1, unsigned long x2,
                    unsigned long x3, unsigned long x4)
{
    plat_handle_fiq_irq();
    smc_call(SMC_FIQ_DONE, 0, 0, 0, 0);
}

int sys_tee_wait_switch_req(struct smc_registers *regs_u)
{
    int ret;
    struct smc_percpu_struct *percpu;
    // kinfo("%s %d\n", __func__, __LINE__);

    percpu = &smc_percpu_structs;

    lock(&smc_struct_lock);

    BUG_ON(percpu->waiting_thread);

    percpu->waiting_thread = current_thread;
    percpu->regs_u = regs_u;

    current_thread->thread_ctx->state = TS_WAITING;

    unlock(&smc_struct_lock);

    sched();
    eret_to_thread(switch_context());
    BUG("Should not reach here.\n");
}

bool not_first_smc[PLAT_CPU_NUM];

unsigned long sys_tee_switch_req(struct smc_registers *regs_u)
{
    int ret;
    struct smc_registers regs_k;

    // kinfo("%s %d\n", __func__, __LINE__);

#ifdef HIGH_SECURE_DEBUG
    int diff_ctr = 0;
    for (unsigned long i = 0; i < 0x100000 / 4096; ++i) {
        int diff_page = 0;
        for (unsigned j = 0; j < 4096; ++j) {
            if (*(unsigned char*)phys_to_virt(0x20000000 + i*4096+j) != *((unsigned char*)phys_to_virt(i*4096 + j))) {
                diff_ctr++;
                diff_page = 1;
                // kinfo("zzh: diff at addr 0x%lx\n", i);
            }
        }
        if (diff_page) {
            kinfo("zzh: page %d different\n", i);
        }
    }
    kinfo("zzh: total %d byte different\n", diff_ctr);
#endif

    ret = copy_from_user(&regs_k, regs_u, sizeof(regs_k));
    BUG_ON(ret);

    bool enqueue = true;

    if (not_first_smc[smp_get_cpu_id()]) {
        regs_k.x0 = SMC_STD_RESPONSE;
        if (regs_k.x1 != SMC_EXIT_SHADOW) {
            regs_k.x1 = SMC_EXIT_NORMAL;
        } else {
            regs_k.x4 = (unsigned long)current_thread;
            /* TEMP DIAGNOSTIC: pairs with the wake-side trace above and
             * [TZLLM_TRACE] in tc_client_driver.c -- logs every thread that
             * exits ChCore via SMC_EXIT_SHADOW (push_pages/pop_pages), so a
             * stalled tensor load can be matched: does this thread's exit
             * ever get a matching wake in handle_yield_smc? */
            static int shadow_exit_ctr = 0;
            static int io_poll_ctr = 0;
            ++shadow_exit_ctr;
            /* x2==4 is the io_rpc()/__io_try_get() poll (see io-frontend.cpp)
             * -- fires hundreds of times/sec while waiting, drowning out the
             * real push(x2=1)/pop(x2=0) events. Throttle it hard; keep every
             * push/pop occurrence since those are rare and are the ones that
             * matter for finding where the tensor-load sequence stalls. */
            if (regs_k.x2 == 4) {
                if ((++io_poll_ctr % 2000) != 1)
                    goto skip_trace_print;
            }
            kinfo("[TZLLM_TRACE] sys_tee_switch_req: SMC_EXIT_SHADOW thread %p x2=%lx x3=%lx (#%d, poll#%d)\n",
                current_thread, regs_k.x2, regs_k.x3, shadow_exit_ctr, io_poll_ctr);
skip_trace_print:;
            enqueue = false;
        }
    } else {
        if (smp_get_cpu_id() == 0) {
            kinfo("cpu 0 SMC_ENTRY_DONE\n");
            regs_k.x0 = SMC_ENTRY_DONE;
            regs_k.x1 = (vaddr_t)&tz_vectors;
        } else {
            // kinfo("cpu %d SMC_ON_DONE\n", smp_get_cpu_id());
            regs_k.x0 = SMC_ON_DONE;
            regs_k.x1 = 0;
        }
        not_first_smc[smp_get_cpu_id()] = true;
    }

    arch_set_thread_return(current_thread, 0);
    current_thread->thread_ctx->state = TS_INTER;
    current_thread->thread_ctx->kernel_stack_state = KS_FREE;
    if (enqueue) BUG_ON(sched_enqueue(current_thread));
    current_thread = NULL;

#if FPU_SAVING_MODE == LAZY_FPU_MODE
    change_fpu_owner_to_ree();
#endif
    smc_call(regs_k.x0, regs_k.x1, regs_k.x2, regs_k.x3, regs_k.x4);
    BUG("Should not reach here.\n");
}

void smc_idle_thread_routine(void)
{
    BUG("%s %d\n", __func__, __LINE__);
}

int sys_tee_pull_kernel_var(kernel_shared_varibles_t *kernel_var_ubuf)
{
    int ret;

    kinfo("%s\n", __func__);

    if (check_user_addr_range((vaddr_t)kernel_var_ubuf,
                              sizeof(kernel_shared_varibles_t))) {
        return -EINVAL;
    }

    ret = copy_to_user(
        kernel_var_ubuf, &kernel_var, sizeof(kernel_shared_varibles_t));

    return ret;
}