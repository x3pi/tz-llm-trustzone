#include <arch/tools.h>
#include <arch/mmu.h>
#include <machine.h>

static inline void set32(vaddr_t addr, u32 set_mask)
{
    put32(addr, get32(addr) | set_mask);
}
static inline void clr32(vaddr_t addr, u32 set_mask)
{
    put32(addr, get32(addr) & ~set_mask);
}

void switch_firewall_ddr(int secure, int core_index)
{
    vaddr_t firewall_ddr_vaddr = phys_to_virt(FIREWALL_DDR_BASE);

    if (secure) {
        /* 3 npus */
        if (core_index == 2) clr32(firewall_ddr_vaddr + FIREWALL_DDR_MST(2), 0xffff0000);
        if (core_index == 0) clr32(firewall_ddr_vaddr + FIREWALL_DDR_MST(11), 0xffff0000);
        if (core_index == 1) clr32(firewall_ddr_vaddr + FIREWALL_DDR_MST(32), 0xffff0000);
        /* master40 */
        if (core_index == 0) clr32(firewall_ddr_vaddr + FIREWALL_DDR_MST(20), 0x0000ffff);
    } else {
        /* 3 npus */
        if (core_index == 2) set32(firewall_ddr_vaddr + FIREWALL_DDR_MST(2), 0xffff0000);
        if (core_index == 0) set32(firewall_ddr_vaddr + FIREWALL_DDR_MST(11), 0xffff0000);
        if (core_index == 1) set32(firewall_ddr_vaddr + FIREWALL_DDR_MST(32), 0xffff0000);
        /* master40 */
        if (core_index == 0) set32(firewall_ddr_vaddr + FIREWALL_DDR_MST(20), 0x0000ffff);
    }
}

/* unit: Mb */
int dsu_fw_rgn_alter(unsigned long base_mb, unsigned long top_mb, int rgn_id)
{
    vaddr_t firewall_dsu_base = phys_to_virt(FIREWALL_DSU_BASE);

	if (rgn_id >= FIREWALL_DSU_RGN_CNT || rgn_id < 0) {
        return -1;
	}

    kinfo("%s %d\n", __func__, __LINE__);

	put32(firewall_dsu_base + FIREWALL_DSU_RGN(rgn_id),
		      RG_MAP_SECURE(top_mb, base_mb));

    kinfo("%s %d\n", __func__, __LINE__);

    return 0;
}

int dsu_fw_rgn_enable(int rgn_id, bool enable)
{
	int i;
    vaddr_t firewall_dsu_base = phys_to_virt(FIREWALL_DSU_BASE);

	if (rgn_id >= FIREWALL_DSU_RGN_CNT || rgn_id < 0) {
        return -1;
	}

    kinfo("%s %d\n", __func__, __LINE__);
    
    for (i = 0; i < DDR_CHN_CNT; i++) {
        if (enable) {
            set32(firewall_dsu_base + FIREWALL_DSU_CON(i), BIT(rgn_id));
        } else {
            clr32(firewall_dsu_base + FIREWALL_DSU_CON(i), BIT(rgn_id));
        }
    }

    kinfo("%s %d\n", __func__, __LINE__);

    return 0;
}

/* unit: Mb */
int ddr_fw_rgn_alter(unsigned long base_mb, unsigned long top_mb, int rgn_id)
{
    vaddr_t firewall_ddr_base = phys_to_virt(FIREWALL_DDR_BASE);

    if (rgn_id >= FIREWALL_DDR_RGN_CNT || rgn_id < 0) {
        return -1;
    }

    kinfo("%s %d\n", __func__, __LINE__);

    put32(firewall_ddr_base + FIREWALL_DDR_RGN(rgn_id),
                RG_MAP_SECURE(top_mb, base_mb));

    kinfo("%s %d\n", __func__, __LINE__);

    return 0;
}

int ddr_fw_rgn_enable(int rgn_id, bool enable) {
    vaddr_t firewall_ddr_base = phys_to_virt(FIREWALL_DDR_BASE);
    if (rgn_id >= FIREWALL_DDR_RGN_CNT || rgn_id < 0) {
        return -1;
    }

    kinfo("%s %d\n", __func__, __LINE__);

    if (enable) {
        /* enable region */
        set32(firewall_ddr_base + FIREWALL_DDR_CON, BIT(rgn_id));
    } else {
        clr32(firewall_ddr_base + FIREWALL_DDR_CON, BIT(rgn_id));
    }

    kinfo("%s %d\n", __func__, __LINE__);

    return 0;
}

void firewall_ddr_cma_rgn_init(void) {
    kinfo("%s %d\n", __func__, __LINE__);
    return;
    for (int rgn = 0; rgn < 4; rgn++) {
        kinfo("%s %d\n", __func__, __LINE__);
        dsu_fw_rgn_alter(0, 0, rgn + 8);
        kinfo("%s %d\n", __func__, __LINE__);
        dsu_fw_rgn_enable(rgn + 8, true);
        kinfo("%s %d\n", __func__, __LINE__);
        ddr_fw_rgn_alter(0, 0, rgn + 8);
        kinfo("%s %d\n", __func__, __LINE__);
        ddr_fw_rgn_enable(rgn + 8, true);
        kinfo("%s %d\n", __func__, __LINE__);
    }
    kinfo("%s %d\n", __func__, __LINE__);
}
