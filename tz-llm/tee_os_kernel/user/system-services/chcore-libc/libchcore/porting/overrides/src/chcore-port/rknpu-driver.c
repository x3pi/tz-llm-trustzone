#include "rknpu-driver.h"
#include <stdlib.h>
#include <assert.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>

#define RKNPU_MMIO_SIZE 0x10000
#define RKNPU_MAX_CORES 3
#define RKNPU_CORE0_MASK 0x01
#define RKNPU_CORE1_MASK 0x02
#define RKNPU_CORE2_MASK 0x04

#define RK_MMU_NUM           (4)
#define RK_MMU_MMIO_SIZE     (0x100)

/** MMU register offsets */
#define RK_MMU_DTE_ADDR		0x00	/* Directory table address */
#define RK_MMU_STATUS		0x04
#define RK_MMU_COMMAND		0x08
#define RK_MMU_PAGE_FAULT_ADDR	0x0C	/* IOVA of last page fault */
#define RK_MMU_ZAP_ONE_LINE	0x10	/* Shootdown one IOTLB entry */
#define RK_MMU_INT_RAWSTAT	0x14	/* IRQ status ignoring mask */
#define RK_MMU_INT_CLEAR	0x18	/* Acknowledge and re-arm irq */
#define RK_MMU_INT_MASK		0x1C	/* IRQ enable */
#define RK_MMU_INT_STATUS	0x20	/* IRQ status after masking */
#define RK_MMU_AUTO_GATING	0x24

#define RK_MMU_CMD_ENABLE_PAGING    0  /* Enable memory translation */
#define RK_MMU_CMD_DISABLE_PAGING   1  /* Disable memory translation */
#define RK_MMU_CMD_ENABLE_STALL     2  /* Stall paging to allow other cmds */
#define RK_MMU_CMD_DISABLE_STALL    3  /* Stop stall re-enables paging */
#define RK_MMU_CMD_ZAP_CACHE        4  /* Shoot down entire IOTLB */
#define RK_MMU_CMD_PAGE_FAULT_DONE  5  /* Clear page fault */
#define RK_MMU_CMD_FORCE_RESET      6  /* Reset all registers */

#define BIT(nr) (1UL << (nr))

/* RK_MMU_STATUS fields */
#define RK_MMU_STATUS_PAGING_ENABLED       BIT(0)
#define RK_MMU_STATUS_PAGE_FAULT_ACTIVE    BIT(1)
#define RK_MMU_STATUS_STALL_ACTIVE         BIT(2)
#define RK_MMU_STATUS_IDLE                 BIT(3)
#define RK_MMU_STATUS_REPLAY_BUFFER_EMPTY  BIT(4)
#define RK_MMU_STATUS_PAGE_FAULT_IS_WRITE  BIT(5)
#define RK_MMU_STATUS_STALL_NOT_ACTIVE     BIT(31)


/* RK_MMU_INT_* register fields */
#define RK_MMU_IRQ_PAGE_FAULT    0x01  /* page fault */
#define RK_MMU_IRQ_BUS_ERROR     0x02  /* bus read error */
#define RK_MMU_IRQ_MASK          (RK_MMU_IRQ_PAGE_FAULT | RK_MMU_IRQ_BUS_ERROR)

#define NUM_DT_ENTRIES 1024
#define NUM_PT_ENTRIES 1024

#define SPAGE_ORDER 12
#define SPAGE_SIZE (1 << SPAGE_ORDER)

#define PT_MEM_SIZE (1UL * SPAGE_SIZE * NUM_PT_ENTRIES)
#define DT_MEM_SIZE (1UL * PT_MEM_SIZE * NUM_DT_ENTRIES)

#define DISABLE_FETCH_DTE_TIME_LIMIT BIT(31)

#define CMD_RETRY_COUNT 10

 /*
  * Support mapping any size that fits in one page table:
  *   4 KiB to 4 MiB
  */
#define RK_IOMMU_PGSIZE_BITMAP 0x007ff000

#define DT_LO_MASK 0xfffff000
#define DT_HI_MASK GENMASK_ULL(39, 32)
#define DT_SHIFT   28

#define DTE_BASE_HI_MASK GENMASK_ULL(11, 4)

#define PAGE_DESC_LO_MASK   0xfffff000
#define PAGE_DESC_HI1_LOWER 32
#define PAGE_DESC_HI1_UPPER 35
#define PAGE_DESC_HI2_LOWER 36
#define PAGE_DESC_HI2_UPPER 39
#define PAGE_DESC_HI_MASK1  GENMASK_ULL(PAGE_DESC_HI1_UPPER, PAGE_DESC_HI1_LOWER)
#define PAGE_DESC_HI_MASK2  GENMASK_ULL(PAGE_DESC_HI2_UPPER, PAGE_DESC_HI2_LOWER)

#define DTE_HI1_LOWER 8
#define DTE_HI1_UPPER 11
#define DTE_HI2_LOWER 4
#define DTE_HI2_UPPER 7
#define DTE_HI_MASK1  GENMASK_ULL(DTE_HI1_UPPER, DTE_HI1_LOWER)
#define DTE_HI_MASK2  GENMASK_ULL(DTE_HI2_UPPER, DTE_HI2_LOWER)

#define PAGE_DESC_HI_SHIFT1 (PAGE_DESC_HI1_LOWER - DTE_HI1_LOWER)
#define PAGE_DESC_HI_SHIFT2 (PAGE_DESC_HI2_LOWER - DTE_HI2_LOWER)

#define GENMASK_ULL(h, l) (((~0ULL) >> (63 - (h))) & (~0ULL << (l)))

struct rknpu_config {
	// __u32 bw_priority_addr;
	// __u32 bw_priority_length;
	// __u64 dma_mask;
	unsigned pc_data_amount_scale;
    unsigned pc_task_number_bits;
	unsigned pc_task_number_mask;
	unsigned pc_task_status_offset;
	// __u32 pc_dma_ctrl;
	// const struct rknpu_irqs_data *irqs;
	// int num_irqs;
	// __u64 nbuf_phyaddr;
	// __u64 nbuf_size;
	unsigned long max_submit_number;
	// __u32 core_mask;
	// const struct rknpu_amount_data *amount_top;
	// const struct rknpu_amount_data *amount_core;
	// void (*state_init)(struct rknpu_device *rknpu_dev);
	// int (*cache_sgt_init)(struct rknpu_device *rknpu_dev);
};

struct rknpu_device {
    void *base[RKNPU_MAX_CORES];
    cap_t notif[RKNPU_MAX_CORES];
    const struct rknpu_config *config;
};

struct rknpu_mem_object {
	// unsigned long flags;
	// unsigned long size;
	void *kv_addr;
	// dma_addr_t dma_addr;
	// struct page **pages;
	// struct sg_table *sgt;
	// struct dma_buf *dmabuf;
	// struct list_head head;
	// unsigned int owner;
};

static const struct rknpu_config rk3588_rknpu_config = {
	// .bw_priority_addr = 0x0,
	// .bw_priority_length = 0x0,
	// .dma_mask = DMA_BIT_MASK(40),
	.pc_data_amount_scale = 2,
	.pc_task_number_bits = 12,
	.pc_task_number_mask = 0xfff,
	.pc_task_status_offset = 0x3c,
	// .pc_dma_ctrl = 0,
	// .irqs = rk3588_npu_irqs,
	// .num_irqs = ARRAY_SIZE(rk3588_npu_irqs),
	// .nbuf_phyaddr = 0,
	// .nbuf_size = 0,
	.max_submit_number = (1 << 12) - 1,
	// .core_mask = 0x7,
	// .amount_top = NULL,
	// .amount_core = NULL,
	// .state_init = NULL,
	// .cache_sgt_init = NULL,
};

static inline unsigned readl(const volatile void *addr)
{
    return *(volatile unsigned *)addr;
}

static inline void writel(unsigned val, volatile void *addr)
{
    *(volatile unsigned *)addr = val;
}

static inline unsigned reg_read(void *base, unsigned offset, const char *func, int line) {
	unsigned value = readl(base + offset);
	// printf("read: %s %d %lx %#x %#x\n", func, line, (unsigned long)base, offset, value);
	return value;
}
static inline void reg_write(void *base, unsigned offset, unsigned value, const char *func, int line) {
	// printf("write: %s %d %#lx %#x %#x\n", func, line, (unsigned long)base, offset, value);
	writel(value, base + offset);
}

#define _REG_READ(base, offset) reg_read((void *)base, (unsigned)offset, __func__, __LINE__)
#define _REG_WRITE(base, value, offset) reg_write((void *)base, (unsigned)offset, (unsigned)value, __func__, __LINE__)

#define _REG_READ2(base, offset) readl(base + (offset))
#define REG_READ2(offset) _REG_READ2(rknpu_core_base, offset)
#define REG_READ(offset) _REG_READ(rknpu_core_base, offset)
#define REG_WRITE(value, offset) _REG_WRITE(rknpu_core_base, value, offset)

#define RK_DTE_PT_VALID           BIT(0)

/*
 * In v2:
 * 31:12 - PT address bit 31:0
 * 11: 8 - PT address bit 35:32
 *  7: 4 - PT address bit 39:36
 *  3: 1 - Reserved
 *     0 - 1 if PT @ PT address is valid
 */
#define RK_DTE_PT_ADDRESS_MASK_V2 0xfffffff0

/*
 * rk3288 iova (IOMMU Virtual Address) format
 *  31       22.21       12.11          0
 * +-----------+-----------+-------------+
 * | DTE index | PTE index | Page offset |
 * +-----------+-----------+-------------+
 *  31:22 - DTE index   - index of DTE in DT
 *  21:12 - PTE index   - index of PTE in PT @ DTE.pt_address
 *  11: 0 - Page offset - offset into page @ PTE.page_address
 */
#define RK_IOVA_DTE_MASK    0xffc00000
#define RK_IOVA_DTE_SHIFT   22
#define RK_IOVA_PTE_MASK    0x003ff000
#define RK_IOVA_PTE_SHIFT   12
#define RK_IOVA_PAGE_MASK   0x00000fff
#define RK_IOVA_PAGE_SHIFT  0

struct rk_iommu_domain {
	// struct list_head iommus;
	u32 *dt; /* page directory table */
	paddr_t dt_dma;
	// spinlock_t iommus_lock; /* lock for iommus list */
	pthread_mutex_t dt_lock; /* lock for modifying page directory table */
	// bool shootdown_entire;

	// struct iommu_domain domain;

	u32 * vdt[NUM_DT_ENTRIES];
};

static u32 rk_iova_dte_index(paddr_t iova)
{
	return (u32)(iova & RK_IOVA_DTE_MASK) >> RK_IOVA_DTE_SHIFT;
}

static u32 rk_iova_pte_index(paddr_t iova)
{
	return (u32)(iova & RK_IOVA_PTE_MASK) >> RK_IOVA_PTE_SHIFT;
}

static u32 rk_iova_page_offset(paddr_t iova)
{
	return (u32)(iova & RK_IOVA_PAGE_MASK) >> RK_IOVA_PAGE_SHIFT;
}

static inline paddr_t rk_dte_pt_address_v2(u32 dte)
{
	u64 dte_v2 = dte;

	dte_v2 = ((dte_v2 & DTE_HI_MASK2) << PAGE_DESC_HI_SHIFT2) |
		 ((dte_v2 & DTE_HI_MASK1) << PAGE_DESC_HI_SHIFT1) |
		 (dte_v2 & PAGE_DESC_LO_MASK);

	return (paddr_t)dte_v2;
}

static inline bool rk_dte_is_pt_valid(u32 dte)
{
	return dte & RK_DTE_PT_VALID;
}

static inline u32 rk_mk_dte_v2(unsigned long pt_dma)
{
	pt_dma = (pt_dma & PAGE_DESC_LO_MASK) |
		 ((pt_dma & PAGE_DESC_HI_MASK1) >> PAGE_DESC_HI_SHIFT1) |
		 (pt_dma & PAGE_DESC_HI_MASK2) >> PAGE_DESC_HI_SHIFT2;

	return (pt_dma & RK_DTE_PT_ADDRESS_MASK_V2) | RK_DTE_PT_VALID;
}

#define RK_PTE_PAGE_VALID         BIT(0)
/*
 * In v2:
 * 31:12 - Page address bit 31:0
 *  11:9 - Page address bit 34:32
 *   8:4 - Page address bit 39:35
 *     3 - Security
 *     2 - Writable
 *     1 - Readable
 *     0 - 1 if Page @ Page address is valid
 */
#define RK_PTE_PAGE_ADDRESS_MASK_V2  0xfffffff0
#define RK_PTE_PAGE_FLAGS_MASK_V2    0x0000000e
#define RK_PTE_PAGE_READABLE_V2      BIT(1)
#define RK_PTE_PAGE_WRITABLE_V2      BIT(2)

#define RK_PTE_PAGE_REPRESENT	BIT(3)

#define IOMMU_READ	(1 << 0)
#define IOMMU_WRITE	(1 << 1)

static inline paddr_t rk_pte_page_address_v2(u32 pte)
{
	u64 pte_v2 = pte;

	pte_v2 = ((pte_v2 & DTE_HI_MASK2) << PAGE_DESC_HI_SHIFT2) |
		 ((pte_v2 & DTE_HI_MASK1) << PAGE_DESC_HI_SHIFT1) |
		 (pte_v2 & PAGE_DESC_LO_MASK);

	return (paddr_t)pte_v2;
}

static inline bool rk_pte_is_page_valid(u32 pte)
{
	return pte & RK_PTE_PAGE_VALID;
}

static inline bool rk_pte_is_page_represent(u32 pte)
{
	return pte & RK_PTE_PAGE_REPRESENT;
}

static u32 rk_mk_pte_v2(paddr_t page, int prot)
{
	u32 flags = 0;

	flags |= (prot & IOMMU_READ) ? RK_PTE_PAGE_READABLE_V2 : 0;
	flags |= (prot & IOMMU_WRITE) ? RK_PTE_PAGE_WRITABLE_V2 : 0;
	/* If BIT(3) set, don't break iommu_map if BIT(0) set.
	 * Means we can reupdate a page that already presented. We can use
	 * this bit to reupdate a pre-mapped 4G range.
	 */
	// flags |= (prot & IOMMU_PRIV) ? RK_PTE_PAGE_REPRESENT : 0;

	page = (page & PAGE_DESC_LO_MASK) |
	       ((page & PAGE_DESC_HI_MASK1) >> PAGE_DESC_HI_SHIFT1) |
	       (page & PAGE_DESC_HI_MASK2) >> PAGE_DESC_HI_SHIFT2;
	page &= RK_PTE_PAGE_ADDRESS_MASK_V2;

	return page | flags | RK_PTE_PAGE_VALID;
}

static u32 *rk_dte_get_page_table_v2(struct rk_iommu_domain *rk_domain, vaddr_t iova)
{
	u32 *page_table, *dte_addr;
	u32 dte_index, dte;
	paddr_t pt_dma;

	// assert_spin_locked(&rk_domain->dt_lock);

	dte_index = rk_iova_dte_index(iova);
	assert(dte_index < NUM_DT_ENTRIES);
	dte_addr = &rk_domain->dt[dte_index];
	dte = *dte_addr;
	if (rk_dte_is_pt_valid(dte))
		goto done;

	// page_table = (u32 *)get_zeroed_page(GFP_ATOMIC | GFP_DMA32);
	struct chcore_dma_handle dma_handle;
	page_table = chcore_alloc_dma_mem(PAGE_SIZE, &dma_handle, false);
	assert(page_table);
	memset(page_table, 0, PAGE_SIZE);

	pt_dma = dma_handle.paddr;
	rk_domain->vdt[dte_index] = (u32 *)page_table;

	dte = rk_mk_dte_v2(pt_dma);
	*dte_addr = dte;
done:
	return rk_domain->vdt[dte_index];
}

static int rk_iommu_map_iova_v2(struct rk_iommu_domain *rk_domain, u32 *pte_addr,
				unsigned long pte_dma, unsigned long iova,
				paddr_t paddr, size_t size, int prot)
{
	unsigned int pte_count;
	unsigned int pte_total = size / SPAGE_SIZE;
	paddr_t page_phys;

	// assert_spin_locked(&rk_domain->dt_lock);

	for (pte_count = 0; pte_count < pte_total; pte_count++) {
		u32 pte = pte_addr[pte_count];

		assert(!(rk_pte_is_page_valid(pte) && !rk_pte_is_page_represent(pte)));

		// if (prot & IOMMU_PRIV) {
		// 	pte_addr[pte_count] = rk_mk_pte_v2(res_page, prot);
		// } else {
			assert(pte_count < NUM_PT_ENTRIES);
			pte_addr[pte_count] = rk_mk_pte_v2(paddr, prot);
			paddr += SPAGE_SIZE;
		// }
	}

	// rk_table_flush(rk_domain, pte_dma, pte_total);

	/*
	 * Zap the first and last iova to evict from iotlb any previously
	 * mapped cachelines holding stale values for its dte and pte.
	 * We only zap the first and last iova, since only they could have
	 * dte or pte shared with an existing mapping.
	 */
	/* Do not zap tlb cache line if shootdown_entire set */
	// if (!rk_domain->shootdown_entire)
	// 	rk_iommu_zap_iova_first_last(rk_domain, iova, size);

	return 0;
// unwind:
	/* Unmap the range of iovas that we just mapped */
	// rk_iommu_unmap_iova(rk_domain, pte_addr, pte_dma,
	// 		    pte_count * SPAGE_SIZE, NULL);

	// iova += pte_count * SPAGE_SIZE;
	// page_phys = rk_pte_page_address_v2(pte_addr[pte_count]);
	// pr_err("iova: %pad already mapped to %pa cannot remap to phys: %pa prot: %#x\n",
	//        &iova, &page_phys, &paddr, prot);

	// return -EADDRINUSE;
}

static int rk_iommu_map_v2(struct rk_iommu_domain *rk_domain, unsigned long _iova,
			paddr_t paddr, size_t size, int prot)
{
	unsigned long flags;
	unsigned long pte_dma, iova;
	u32 *page_table, *pte_addr;
	u32 dte, pte_index;
	int ret;

	// spin_lock_irqsave(&rk_domain->dt_lock, flags);
	pthread_mutex_lock(&rk_domain->dt_lock);

	for (iova = _iova; iova < _iova + size;) {
		// printf("%s %d %lx %d %d\n", __func__, __LINE__, iova, rk_iova_dte_index(iova), rk_iova_pte_index(iova));
		page_table = rk_dte_get_page_table_v2(rk_domain, iova);
		assert(page_table);
	
		dte = rk_domain->dt[rk_iova_dte_index(iova)];
		pte_index = rk_iova_pte_index(iova);
		pte_addr = &page_table[pte_index];
		pte_dma = rk_dte_pt_address_v2(dte) + pte_index * sizeof(u32);
		unsigned long next_iova = (iova + PT_MEM_SIZE) / PT_MEM_SIZE * PT_MEM_SIZE;
		size_t cur_size = (size < (next_iova - iova) ? size : (next_iova - iova));
		ret = rk_iommu_map_iova_v2(rk_domain, pte_addr, pte_dma, iova,
					   paddr, cur_size, prot);
		iova = next_iova;
		paddr += cur_size;
	}

	// spin_unlock_irqrestore(&rk_domain->dt_lock, flags);
	pthread_mutex_unlock(&rk_domain->dt_lock);

	return ret;
}

static u32 rk_iommu_read(void *base, u32 offset)
{
	return readl(base + offset);
}

static void rk_iommu_write(void *base, u32 offset, u32 value)
{
	writel(value, base + offset);
}

static void rk_iommu_command(void *base[], u32 command)
{
	int i;
	for (i = 0; i < RK_MMU_NUM; i++)
		writel(command, base[i] + RK_MMU_COMMAND);
}

static bool rk_iommu_is_stall_active(void *base[])
{
	bool active = true;
	int i;

	for (i = 0; i < RK_MMU_NUM; i++)
		active &= !!(rk_iommu_read(base[i], RK_MMU_STATUS) &
					   RK_MMU_STATUS_STALL_ACTIVE);

	return active;
}

static bool rk_iommu_is_paging_enabled(void *base[])
{
	bool enable = true;
	int i;

	for (i = 0; i < RK_MMU_NUM; i++)
		enable &= !!(rk_iommu_read(base[i], RK_MMU_STATUS) &
					   RK_MMU_STATUS_PAGING_ENABLED);

	return enable;
}

#define IOMMU_DOMAIN_NUM (3)

cap_t mmu_pmo_cap[RK_MMU_NUM];
void *mmu_base[RK_MMU_NUM];
unsigned int int_mask[RK_MMU_NUM];
unsigned int dte_addr[RK_MMU_NUM];
struct rk_iommu_domain rk_domains[IOMMU_DOMAIN_NUM];

int rknpu_mmu_init(unsigned long iommu_base_paddr[]) {
	int i;

	for (i = 0; i < RK_MMU_NUM; i++) {
		mmu_pmo_cap[i] = usys_create_device_pmo(iommu_base_paddr[i], RK_MMU_MMIO_SIZE);
		assert(mmu_pmo_cap[i] >= 0);

		mmu_base[i] = chcore_auto_map_pmo(mmu_pmo_cap[i], RKNPU_MMIO_SIZE, VM_READ | VM_WRITE);
		assert(mmu_base[i]);
	}

	// for (i = 0; i < RK_MMU_NUM; i++) {
	// 	chcore_auto_unmap_pmo(mmu_pmo_cap[i], (unsigned long)mmu_base[i], RK_MMU_MMIO_SIZE);
	// 	usys_revoke_cap(mmu_pmo_cap[i], true);
	// }
	/* TODO: move out of TTFT critical path */
	// for (i = 0; i < IOMMU_DOMAIN_NUM; i++) {
	// 	struct chcore_dma_handle dma_handle;
	// 	rk_domains[i].dt = chcore_alloc_dma_mem(PAGE_SIZE, &dma_handle, false);
	// 	memset(rk_domains[i].dt, 0, PAGE_SIZE);
	// 	rk_domains[i].dt_dma = dma_handle.paddr;
	// 	rk_iommu_map_v2(rk_domains + i, 0, i * DT_MEM_SIZE, DT_MEM_SIZE, IOMMU_READ | IOMMU_WRITE);
	// }
}

void switch_domain(struct rk_iommu_domain *rk_domain) {
	int i;

	if (!rk_iommu_is_stall_active(mmu_base) && rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_ENABLE_STALL);
		while (!rk_iommu_is_stall_active(mmu_base));
	}

	if (rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_DISABLE_PAGING);
		while (rk_iommu_is_paging_enabled(mmu_base));
	}

	for (i = 0; i < RK_MMU_NUM; i++) {
		u32 dt_v2;
		dt_v2 = (rk_domain->dt_dma & DT_LO_MASK) |
				((rk_domain->dt_dma & DT_HI_MASK) >> DT_SHIFT);
		writel(0, mmu_base[i] + RK_MMU_INT_MASK);
		writel(dt_v2, mmu_base[i] + RK_MMU_DTE_ADDR);
		/* TODO: flush iotlb? */
	}

	if (!rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_ENABLE_PAGING);
		while (!rk_iommu_is_paging_enabled(mmu_base));
	}

	if (rk_iommu_is_stall_active(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_DISABLE_STALL);
		while (rk_iommu_is_stall_active(mmu_base));
	}
}

void disable_mmu(void) {
	int i;

	if (!rk_iommu_is_stall_active(mmu_base) && rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_ENABLE_STALL);
		while (!rk_iommu_is_stall_active(mmu_base));
	}

	if (rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_DISABLE_PAGING);
		while (rk_iommu_is_paging_enabled(mmu_base));
	}

	for (i = 0; i < RK_MMU_NUM; i++) {
		int_mask[i] = readl(mmu_base[i] + RK_MMU_INT_MASK);
		dte_addr[i] = readl(mmu_base[i] + RK_MMU_DTE_ADDR);
		writel(0, mmu_base[i] + RK_MMU_INT_MASK);
		writel(0, mmu_base[i] + RK_MMU_DTE_ADDR);
	}

	if (rk_iommu_is_stall_active(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_DISABLE_STALL);
		while (rk_iommu_is_stall_active(mmu_base));
	}
}

void enable_mmu(void) {
	int i;

	if (!rk_iommu_is_stall_active(mmu_base) && rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_ENABLE_STALL);
		while (!rk_iommu_is_stall_active(mmu_base));
	}

	if (!rk_iommu_is_paging_enabled(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_ENABLE_PAGING);
		while (!rk_iommu_is_paging_enabled(mmu_base));
	}

	for (i = 0; i < RK_MMU_NUM; i++) {
		writel(int_mask[i], mmu_base[i] + RK_MMU_INT_MASK);
		writel(dte_addr[i], mmu_base[i] + RK_MMU_DTE_ADDR);
	}

	if (rk_iommu_is_stall_active(mmu_base)) {
		rk_iommu_command(mmu_base, RK_MMU_CMD_DISABLE_STALL);
		while (rk_iommu_is_stall_active(mmu_base));
	}
}

#include <pthread.h>
static pthread_mutex_t mtx[RKNPU_MAX_CORES];

#define NPU_MULTIPLEXING
// #define NPU_MULTIPLEXING_MEASURE

unsigned long switch_count;
unsigned long entry_time;
unsigned long exit_time;
unsigned long switch_world_time;
unsigned long tz_config_time;
unsigned long gic_time;

unsigned long iommu_time;

void rknpu_dump_measure(bool clear) {
#ifdef NPU_MULTIPLEXING_MEASURE
    printf("********************************begin rknpu measure dump********************************\n");
	printf("%s:%d: entry %ld us, exit %ld us, count %d\n", __FILE__, __LINE__, entry_time, exit_time, switch_count);
	printf("%s:%d: average entry %ld us, average exit %ld us\n", __FILE__, __LINE__, entry_time / switch_count, exit_time / switch_count);
	printf("%s:%d: switch_world_time %ld us, tz_config_time %ld us, gic_time %ld us\n", __FILE__, __LINE__, switch_world_time, tz_config_time, gic_time);
	printf("%s:%d: iommu_time %ld us\n", __FILE__, __LINE__, iommu_time);
	if (clear) {
		switch_count = entry_time = exit_time = 0;
		switch_world_time = tz_config_time = gic_time = 0;
		iommu_time = 0;
	}
    printf("*********************************end rknpu measure dump*********************************\n");
#endif
}

int rknpu_init(struct rknpu_device **ret_rknpu_dev,
               unsigned long base_paddr[], unsigned long iommu_base_paddr[],
               unsigned long rknpu_irqs[])
{
	struct rknpu_device *rknpu_dev;
	cap_t pmo_cap;
	int core_index;
	void *base;

	rknpu_dev = malloc(sizeof(*rknpu_dev));
	assert(rknpu_dev != NULL);

	rknpu_mmu_init(iommu_base_paddr);
#ifndef NPU_MULTIPLEXING
	disable_mmu();
	// switch_domain(rk_domains + 0);
#if 0
	printf("%s %d\n", __func__, __LINE__);
	for (int i = 0; i < NUM_DT_ENTRIES; i++) {
		paddr_t paddr;
		usys_get_phys_addr(rk_domains[0].vdt[i], &paddr);
		assert(rk_dte_pt_address_v2(rk_domains[0].dt[i]) == paddr);
		for (int j = 0; j < NUM_PT_ENTRIES; j++) {
			assert(rk_pte_page_address_v2(rk_domains[0].vdt[i][j]) == (unsigned long)i * PT_MEM_SIZE + j * SPAGE_SIZE);
		}
	}
#endif
	usys_top(1);
#endif

	for (core_index = 0; core_index < RKNPU_MAX_CORES; core_index++) {
		pthread_mutex_init(&mtx[core_index], NULL);
		rknpu_dev->notif[core_index] = usys_create_npu_irq_notif(base_paddr[core_index], rknpu_irqs[core_index]);
		/* Initialize RKNPU base address */
		pmo_cap = usys_create_device_pmo(base_paddr[core_index], RKNPU_MMIO_SIZE);
		assert(pmo_cap >= 0);
		
		base = chcore_auto_map_pmo(pmo_cap, RKNPU_MMIO_SIZE, VM_READ | VM_WRITE);
		assert(base);

		rknpu_dev->base[core_index] = base;
	}

	rknpu_dev->config = &rk3588_rknpu_config;

	*ret_rknpu_dev = rknpu_dev;
	return 0;
}

// int rknpu_mem_create(struct rknpu_device *rknpu_dev, struct rknpu_mem_create *args)
// {
//     return 0;
// }

// int rknpu_mem_destroy(struct rknpu_device *rknpu_dev, struct rknpu_mem_destroy *args)
// {
//     return 0;
// }

static int core_mask_to_core_index(unsigned core_mask)
{
    switch (core_mask) {
	case RKNPU_CORE0_MASK:
		return 0;
	case RKNPU_CORE1_MASK:
        return 1;
	case RKNPU_CORE2_MASK:
        return 2;
	default:
		assert(0);
		return -1;
	}
}

#ifdef NPU_MULTIPLEXING_MEASURE
#define BEGIN_MEASURE(time) \
	{\
	struct timeval start_time, end_time; \
	gettimeofday(&start_time, NULL);
#else
#define BEGIN_MEASURE(time)
#endif 

#ifdef NPU_MULTIPLEXING_MEASURE
#define END_MEASURE(time) \
	gettimeofday(&end_time, NULL); \
	time += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec; \
}
#else
#define END_MEASURE(time)
#endif

static void *npu_enter_secure(int core_mask) {
	void *job;
	BEGIN_MEASURE(entry_time)
	BEGIN_MEASURE(switch_world_time)
	// printf("%s %d before submit npu task mask %d\n", __func__, __LINE__, args->core_mask);
	struct smc_registers req = {0};
	req.x1 = SMC_EXIT_SHADOW;
	req.x2 = 2;
	req.x3 = core_mask;
	job = (void *)usys_tee_switch_req(&req);
	// printf("%s %d after submit npu task mask %d job %p\n", __func__, __LINE__, args->core_mask, job);
	assert(job != NULL);
	END_MEASURE(switch_world_time)
	// printf("%s %d after submit npu task\n", __func__, __LINE__);
	BEGIN_MEASURE(gic_time)
	usys_top(1 | 4);
	END_MEASURE(gic_time)
	BEGIN_MEASURE(tz_config_time)
	usys_top(1 | 8);
	END_MEASURE(tz_config_time)
	BEGIN_MEASURE(iommu_time)
	disable_mmu();
	END_MEASURE(iommu_time)
	END_MEASURE(entry_time)
	return job;
}

static void npu_exit_secure(void *job) {
	BEGIN_MEASURE(exit_time)
	BEGIN_MEASURE(iommu_time)
	enable_mmu();
	END_MEASURE(iommu_time)
	BEGIN_MEASURE(tz_config_time)
	usys_top(0 | 8);
	END_MEASURE(tz_config_time)
	BEGIN_MEASURE(gic_time)
	usys_top(0 | 4);
	END_MEASURE(gic_time)
	BEGIN_MEASURE(switch_world_time)
	// printf("%s %d before done npu task\n", __func__, __LINE__);
	struct smc_registers req = {0};
	req.x1 = SMC_EXIT_SHADOW;
	req.x2 = 3;
	req.x3 = (unsigned long)job;
	int ret = usys_tee_switch_req(&req);
	assert(ret == 0);
	END_MEASURE(switch_world_time)
	// printf("%s %d after done npu task\n", __func__, __LINE__);
	END_MEASURE(exit_time)
	switch_count++;
}

static int __rknpu_submit(struct rknpu_device *rknpu_dev, struct rknpu_submit *args, bool wait)
{
    int core_index = core_mask_to_core_index(args->core_mask);
	pthread_mutex_lock(&mtx[core_index]);

    void *rknpu_core_base = rknpu_dev->base[core_index];
    
	int task_counter;
    int task_start = args->subcore_task[core_index].task_start;
    int task_number = args->subcore_task[core_index].task_number;
    int task_end = task_start + task_number - 1;
    int max_submit_number = rknpu_dev->config->max_submit_number;
    
    int task_pp_en = args->flags & RKNPU_JOB_PINGPONG ? 1 : 0;
    int pc_args_amount_scale = rknpu_dev->config->pc_data_amount_scale;
	int pc_task_number_bits = rknpu_dev->config->pc_task_number_bits;
	int pc_data_amount_scale = rknpu_dev->config->pc_data_amount_scale;
    
    struct rknpu_task *task_base = (struct rknpu_task *)(uintptr_t)args->task_obj_addr;
	struct rknpu_task *first_task = &task_base[task_start];
	struct rknpu_task *last_task = &task_base[task_end];

    assert(args->task_number <= max_submit_number);

    REG_WRITE(0x1, RKNPU_OFFSET_PC_DATA_ADDR);
    
    REG_WRITE((0xe + 0x10000000 * core_index), 0x1004);
    REG_WRITE((0xe + 0x10000000 * core_index), 0x3004);
    
    REG_WRITE(first_task->regcmd_addr, RKNPU_OFFSET_PC_DATA_ADDR);
    REG_WRITE((first_task->regcfg_amount + RKNPU_PC_DATA_EXTRA_AMOUNT +
		      pc_data_amount_scale - 1) / pc_data_amount_scale - 1,
			  RKNPU_OFFSET_PC_DATA_AMOUNT);

    REG_WRITE(last_task->int_mask, RKNPU_OFFSET_INT_MASK);
	REG_WRITE(first_task->int_mask, RKNPU_OFFSET_INT_CLEAR);
	
	// Disable all interrupts
    // REG_WRITE(0, RKNPU_OFFSET_INT_MASK); 
    // REG_WRITE(0x1ffff, RKNPU_OFFSET_INT_CLEAR);

	REG_WRITE(((0x6 | task_pp_en) << pc_task_number_bits) | task_number,
		      RKNPU_OFFSET_PC_TASK_CONTROL);

	REG_WRITE(args->task_base_addr, RKNPU_OFFSET_PC_DMA_BASE_ADDR);
    
    REG_WRITE(0x1, RKNPU_OFFSET_PC_OP_EN);
	REG_WRITE(0x0, RKNPU_OFFSET_PC_OP_EN);

	if (wait) {
		printf("[NPU Driver] Waiting for IRQ on core %d (notif %d)...\n", core_index, rknpu_dev->notif[core_index]);
		int ret = usys_wait(rknpu_dev->notif[core_index], true, NULL);
		printf("[NPU Driver] Woke up from IRQ on core %d, ret=%d!\n", core_index, ret);
	}

	pthread_mutex_unlock(&mtx[core_index]);
	return 0;
}

int rknpu_submit_multi(struct rknpu_device *rknpu_dev, struct rknpu_submit *args[], int task_num, void *poll) {
	assert(task_num <= RKNPU_MAX_CORES);

#ifdef NPU_MULTIPLEXING
	void *job[task_num];
	for (int core_index = 0; core_index < 1; core_index++) {
		int core_mask = 1 << core_index;
		job[core_index] = npu_enter_secure(core_mask);
	}
#endif

	for (int core_index = 0; core_index < task_num; core_index++) {
		int core_mask = 1 << core_index;
		args[core_index]->core_mask = core_mask;
		int ret = __rknpu_submit(rknpu_dev, args[core_index], false);
		assert(ret == 0);
	}

	for (int core_index = 0; core_index < task_num; core_index++) {
		if (poll) {
			void (*polling_routine)(void) = poll;
			while (1) {
				int ret = usys_wait(rknpu_dev->notif[core_index], false, NULL);
				if (ret == 0) break;
				polling_routine();
			};
		} else {
			printf("[NPU Driver Multi] Waiting for IRQ on core %d...\n", core_index);
			int ret = usys_wait(rknpu_dev->notif[core_index], true, NULL);
			printf("[NPU Driver Multi] Woke up from IRQ on core %d, ret=%d!\n", core_index, ret);
		}
	}
	
#ifdef NPU_MULTIPLEXING
	for (int core_index = 0; core_index < 1; core_index++) {
		int core_mask = 1 << core_index;
		npu_exit_secure(job[core_index]);
	}
#endif

	return 0;
}

int rknpu_submit(struct rknpu_device *rknpu_dev, struct rknpu_submit *args) {
#ifdef NPU_MULTIPLEXING
	void *job;
	job = npu_enter_secure(args->core_mask);
#endif
	int ret = __rknpu_submit(rknpu_dev, args, true);
#ifdef NPU_MULTIPLEXING
	npu_exit_secure(job);
#endif
	return ret;
}

int rknpu_soft_reset(struct rknpu_device *rknpu_dev)
{
    return 0;
}
