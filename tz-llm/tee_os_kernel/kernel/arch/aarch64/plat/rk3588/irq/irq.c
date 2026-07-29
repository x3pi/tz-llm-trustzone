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
#include <arch/tools.h>
#include <arch/mmu.h>
#include <arch/machine/smp.h>
#include <common/types.h>
#include <common/macro.h>
#include <mm/mm.h>
#include <ipc/notification.h>

/* Maximum number of interrups a GIC can support */
#define GIC_MAX_INTS 1020

/* Number of Private Peripheral Interrupt */
#define NUM_PPI	32

/* Number of Software Generated Interrupt */
#define NUM_SGI			16

/* Number of Non-secure Software Generated Interrupt */
#define NUM_NS_SGI		8

/* Number of interrupts in one register */
#define NUM_INTS_PER_REG	32

/* Number of targets in one register */
#define NUM_TARGETS_PER_REG	4

/* Accessors to access ITARGETSRn */
#define ITARGETSR_FIELD_BITS	8
#define ITARGETSR_FIELD_MASK	0xff

/* Offsets from gic.gicd_base */
#define GICD_CTLR          (0x000)
#define GICD_TYPER         (0x004)
#define GICD_IGROUPR(n)    (0x080 + (n)*4)
#define GICD_ISENABLER(n)  (0x100 + (n)*4)
#define GICD_ICENABLER(n)  (0x180 + (n)*4)
#define GICD_ISPENDR(n)    (0x200 + (n)*4)
#define GICD_ICPENDR(n)    (0x280 + (n)*4)
#define GICD_IPRIORITYR(n) (0x400 + (n)*4)
#define GICD_ITARGETSR(n)  (0x800 + (n)*4)
#define GICD_ICFGR(n)      (0xc00 + (n)*4)
#define GICD_IGROUPMODR(n) (0xd00 + (n)*4)
#define GICD_SGIR          (0xF00)

#define GICD_CTLR_ENABLEGRP1S (1 << 2)

/* GICD ICFGR bit fields */
#define GICD_ICFGR_TYPE_EDGE		2
#define GICD_ICFGR_TYPE_LEVEL		0
#define GICD_ICFGR_FIELD_BITS		2
#define GICD_ICFGR_FIELD_MASK		0x3
#define GICD_ICFGR_NUM_INTS_PER_REG	(NUM_INTS_PER_REG / \
					 GICD_ICFGR_FIELD_BITS)

#define IRQ_TYPE_NONE		0
#define IRQ_TYPE_EDGE_RISING	1
#define IRQ_TYPE_EDGE_FALLING	2
#define IRQ_TYPE_EDGE_BOTH	(IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH	4
#define IRQ_TYPE_LEVEL_LOW	8

static inline u32 read_icc_ctlr(void)
{
    u64 val64 = 0;
    asm volatile("mrs %0, "
                 "S3_0_C12_C12_4"
                 : "=r"(val64));
    return val64;
}

static inline void write_icc_ctlr(u32 val)
{
    u64 val64 = val;
    asm volatile("msr "
                 "S3_0_C12_C12_4"
                 ", %0"
                 :
                 : "r"(val64));
}

static inline void write_icc_pmr(u32 val)
{
    u64 val64 = val;
    asm volatile("msr "
                 "S3_0_C4_C6_0"
                 ", %0"
                 :
                 : "r"(val64));
}

static inline void write_icc_igrpen1(u32 val)
{
    u64 val64 = val;
    asm volatile("msr "
                 "S3_0_C12_C12_7"
                 ", %0"
                 :
                 : "r"(val64));
}

static inline void set32(vaddr_t addr, u32 set_mask)
{
    put32(addr, get32(addr) | set_mask);
}
static inline void clr32(vaddr_t addr, u32 set_mask)
{
    put32(addr, get32(addr) & ~set_mask);
}
static inline void mask32(vaddr_t addr, u32 val, u32 mask)
{
	put32(addr, (get32(addr) & ~mask) | (val & mask));
}

static size_t probe_max_it(vaddr_t gicd_base)
{
    int i;
    u32 old_ctlr;
    size_t ret = 0;
    const size_t max_regs =
        ((GIC_MAX_INTS + NUM_INTS_PER_REG - 1) / NUM_INTS_PER_REG) - 1;

    /*
     * Probe which interrupt number is the largest.
     */
    old_ctlr = read_icc_ctlr();
    write_icc_ctlr(0);
    for (i = max_regs; i >= 0; i--) {
        u32 old_reg;
        u32 reg;
        int b;

        old_reg = get32(gicd_base + GICD_ISENABLER(i));
        put32(gicd_base + GICD_ISENABLER(i), 0xffffffff);
        reg = get32(gicd_base + GICD_ISENABLER(i));
        put32(gicd_base + GICD_ICENABLER(i), ~old_reg);
        for (b = NUM_INTS_PER_REG - 1; b >= 0; b--) {
            if ((u32)BIT(b) & reg) {
                ret = i * NUM_INTS_PER_REG + b;
                goto out;
            }
        }
    }
out:
    write_icc_ctlr(old_ctlr);
    return ret;
}

void __plat_interrupt_init_percpu(vaddr_t gicd_base)
{
    /* per-CPU interrupts config:
     * ID0-ID7(SGI)   for Non-secure interrupts
     * ID8-ID15(SGI)  for Secure interrupts.
     * All PPI config as Non-secure interrupts.
     */
    put32(gicd_base + GICD_IGROUPR(0), 0xffff00ff);

    /* Set the priority mask to permit Non-secure interrupts, and to
     * allow the Non-secure world to adjust the priority mask itself
     */
    write_icc_pmr(0x80);
    write_icc_igrpen1(1);
}

void __plat_interrupt_init(vaddr_t gicd_base)
{
    size_t n, max_it;

    max_it = probe_max_it(gicd_base);

    for (n = 0; n <= max_it / NUM_INTS_PER_REG; n++) {
        /* Disable interrupts */
        put32(gicd_base + GICD_ICENABLER(n), 0xffffffff);

        /* Make interrupts non-pending */
        put32(gicd_base + GICD_ICPENDR(n), 0xffffffff);

        /* Mark interrupts non-secure */
        if (n == 0) {
            /* per-CPU inerrupts config:
             * ID0-ID7(SGI)	  for Non-secure interrupts
             * ID8-ID15(SGI)  for Secure interrupts.
             * All PPI config as Non-secure interrupts.
             */
            put32(gicd_base + GICD_IGROUPR(n), 0xffff00ff);
        } else {
            put32(gicd_base + GICD_IGROUPR(n), 0xffffffff);
        }
    }

    write_icc_pmr(0x80);
    write_icc_igrpen1(1);
    set32(gicd_base + GICD_CTLR, GICD_CTLR_ENABLEGRP1S);
}

void plat_interrupt_init(void)
{
    vaddr_t gicd_base;

    gicd_base = phys_to_virt(get_gicd_base());

    if (smp_get_cpu_id() == 0) {
        __plat_interrupt_init(gicd_base);
    } else {
        __plat_interrupt_init_percpu(gicd_base);
    }
}

void irq_add(size_t it)
{
    size_t idx = it / NUM_INTS_PER_REG;
    u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

    /* Disable the interrupt */
    put32(gicd_base + GICD_ICENABLER(idx), mask);
    /* Make it non-pending */
    put32(gicd_base + GICD_ICPENDR(idx), mask);
    /* Assign it to group0 */
    clr32(gicd_base + GICD_IGROUPR(idx), mask);
    /* Assign it to group1S */
    set32(gicd_base + GICD_IGROUPMODR(idx), mask);
}

void irq_rm(size_t it)
{
    size_t idx = it / NUM_INTS_PER_REG;
    u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

    /* Disable the interrupt */
    put32(gicd_base + GICD_ICENABLER(idx), mask);
    /* Make it non-pending */
    put32(gicd_base + GICD_ICPENDR(idx), mask);
    /* Assign it to group0 */
    set32(gicd_base + GICD_IGROUPR(idx), mask);
    /* Assign it to group1S */
    clr32(gicd_base + GICD_IGROUPMODR(idx), mask);
}

void irq_set_cpu_mask(size_t it, u32 cpu_mask)
{
    size_t idx = it / NUM_INTS_PER_REG;
    u32 mask = 1 << (it % NUM_INTS_PER_REG);
    u32 target = 0;
    u32 target_shift = 0;
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());
    vaddr_t itargetsr = gicd_base +
                GICD_ITARGETSR(it / NUM_TARGETS_PER_REG);

    /* Assigned to group0 */
    BUG_ON((get32(gicd_base + GICD_IGROUPR(idx)) & mask));

    /* Route it to selected CPUs */
    target = get32(itargetsr);
    target_shift = (it % NUM_TARGETS_PER_REG) * ITARGETSR_FIELD_BITS;
    target &= ~(ITARGETSR_FIELD_MASK << target_shift);
    target |= cpu_mask << target_shift;
    put32(itargetsr, target);
}

void irq_set_prio(size_t it, u32 prio)
{
	size_t idx = it / NUM_INTS_PER_REG;
	u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

	/* Assigned to group0 */
	// BUG_ON((get32(gicd_base + GICD_IGROUPR(idx)) & mask));

	/* Set prio it to selected CPUs */
    // kdebug("%s %d: %d\n", __func__, __LINE__, get8(gicd_base + GICD_IPRIORITYR(0) + it));
	put8(gicd_base + GICD_IPRIORITYR(0) + it, prio);
}

static void irq_enable(size_t it)
{
	size_t idx = it / NUM_INTS_PER_REG;
	u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

	/* Assigned to group0 */
	// BUG_ON((get32(gicd_base + GICD_IGROUPR(idx)) & mask));

	/* Enable the interrupt */
	put32(gicd_base + GICD_ISENABLER(idx), mask);
}

static void irq_disable(size_t it)
{
	size_t idx = it / NUM_INTS_PER_REG;
	u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

	/* Assigned to group0 */
	BUG_ON((get32(gicd_base + GICD_IGROUPR(idx)) & mask));

	/* Disable the interrupt */
	put32(gicd_base + GICD_ICENABLER(idx), mask);
}

static void irq_set_pending(size_t it)
{
	size_t idx = it / NUM_INTS_PER_REG;
	u32 mask = 1 << (it % NUM_INTS_PER_REG);
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

	/* Should be Peripheral Interrupt */
	BUG_ON(it < NUM_SGI);

	/* Raise the interrupt */
	put32(gicd_base + GICD_ISPENDR(idx), mask);
}

static void irq_set_type(size_t it, u32 type)
{
	size_t index = it / GICD_ICFGR_NUM_INTS_PER_REG;
	u32 shift = (it % GICD_ICFGR_NUM_INTS_PER_REG) *
			 GICD_ICFGR_FIELD_BITS;
	u32 icfg = 0;
    vaddr_t gicd_base = phys_to_virt(get_gicd_base());

	BUG_ON(!(type == IRQ_TYPE_EDGE_RISING || type == IRQ_TYPE_LEVEL_HIGH));

	if (type == IRQ_TYPE_EDGE_RISING)
		icfg = GICD_ICFGR_TYPE_EDGE;
	else
		icfg = GICD_ICFGR_TYPE_LEVEL;

	mask32(gicd_base + GICD_ICFGR(index),
		  (icfg << shift),
		  (GICD_ICFGR_FIELD_MASK << shift));
}

void set_irq_s(size_t it)
{
    irq_add(it);
    // irq_set_cpu_mask(it, 0xff);
    // irq_set_cpu_mask(it, 0x1);
    irq_set_prio(it, 0x1);
    irq_enable(it);
}

void set_irq_ns(size_t it)
{
    irq_rm(it);
    irq_set_prio(it, 208);
    irq_enable(it);
}

void set_npu_irqs_s(void) {
    set_irq_s(142);
    set_irq_s(143);
    set_irq_s(144);
}

void set_npu_irqs_ns(void) {
    set_irq_ns(142);
    set_irq_ns(143);
    set_irq_ns(144);
}

void plat_send_ipi(u32 cpu, u32 ipi)
{
}

void plat_enable_irqno(int irq)
{
}

void plat_disable_irqno(int irq)
{
}

void plat_ack_irq(int irq)
{
}

struct npu_desc {
    vaddr_t npu_base;
    size_t irq;
    struct notification *notif;
};

#define NPU_CORE_NR (3)

static struct npu_desc npu_descs[NPU_CORE_NR];
static int npu_cnt;
static void insert_npu_desc(vaddr_t npu_base, size_t irq, struct notification *notif)
{
    BUG_ON(npu_cnt >= NPU_CORE_NR);
    npu_descs[npu_cnt].npu_base = npu_base;
    npu_descs[npu_cnt].irq = irq;
    npu_descs[npu_cnt].notif = notif;
    npu_cnt++;
}
static struct npu_desc *get_npu_desc(size_t irq) {
    int i;
    for (i = 0; i < npu_cnt; i++)
        if (irq == npu_descs[i].irq)
            return npu_descs + i;
    return NULL;
}

void plat_handle_irq(void)
{
    kinfo("%s %d %d\n", __func__, __LINE__, smp_get_cpu_id());
    unsigned int irqnr = 0;
	unsigned int irqstat = 0;
	int ret;
    struct npu_desc *npu_desc;

	irqstat = read_sys_reg(ICC_IAR1_EL1);
	dsb(sy);
	irqnr = irqstat & 0x3ff;

    npu_desc = get_npu_desc(irqnr);
    BUG_ON(npu_desc == NULL);

    // handle irq of rknpu
    put32(npu_desc->npu_base + 0x24, 0x1ffff);
    kinfo("irq %d handled\n", irqnr);

    write_sys_reg(ICC_EOIR1_EL1, irqnr);

    signal_notific_direct(npu_desc->notif);
}

void plat_handle_fiq_irq(void)
{
    kinfo("%s %d %d\n", __func__, __LINE__, smp_get_cpu_id());
    unsigned int irqnr = 0;
    unsigned int irqstat = 0;
    int ret;
    struct npu_desc *npu_desc;

    irqstat = read_sys_reg(ICC_IAR1_EL1);
    dsb(sy);
    irqnr = irqstat & 0x3ff;

    npu_desc = get_npu_desc(irqnr);
    BUG_ON(npu_desc == NULL);

    // handle irq of rknpu
    put32(npu_desc->npu_base + 0x24, 0x1ffff);
    kinfo("fiq_irq %d handled\n", irqnr);

    write_sys_reg(ICC_EOIR1_EL1, irqnr);

    signal_notific(npu_desc->notif);
}

cap_t sys_create_npu_irq_notif(paddr_t npu_base, size_t irq)
{
    cap_t notif_cap;
    struct notification *notif;

    // set_irq_s(irq);

    // extern void switch_firewall_ddr(int secure, int core_index);
    // extern void switch_secure_device(int secure, int core_index);
    // switch_firewall_ddr(1, 0);
    // switch_firewall_ddr(1, 1);
    // switch_firewall_ddr(1, 2);
    // switch_secure_device(1, 0);
    // switch_secure_device(1, 1);
    // switch_secure_device(1, 2);

    notif_cap = sys_create_notifc();
    BUG_ON(notif < 0);

    notif = obj_get(current_cap_group, notif_cap, TYPE_NOTIFICATION);
    BUG_ON(notif == NULL);

    insert_npu_desc(phys_to_virt(npu_base), irq, notif);

    return notif_cap;
}
