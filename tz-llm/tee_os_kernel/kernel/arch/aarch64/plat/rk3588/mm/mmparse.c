/*
 * Copyright (c) 2024 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <machine.h>
#include <mm/mm.h>
#include <common/macro.h>
#include <common/types.h>
#include <arch/tools.h>
#include <rk_atags.h>

#define FIREWALL_DDR_RGN_CNT 16
#define FIREWALL_DDR_RGN(i) ((i) * 0x4) // *4 means 4 byte (32 bit)
#define FIREWALL_DDR_MST(i)		(0x40 + (i) * 0x4)
#define FIREWALL_DSU_CON(i) (0xf0 + (i) * 4)
#define PMU1GRF_OS_REG(n) (0x200 + ((n) * 4))
#define CENTER_GRF_CON(i) ((i) * 4)
#define SYS_GRF_SOC_CON(n) (0x300 + (n) * 4)
#define FIREWALL_DSU_MST(i) (0x40 + (i) * 0x4)
#define FIREWALL_DDR_CON 0xf0

static char DAT_0005f74c[] = {0x00, 0x20, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
    0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0C,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00};

static unsigned int _DAT_0007cd58, _DAT_0007cd5c;
static int _DAT_0007cd60; // like DDR_CHN_CNT?
static unsigned long _DAT_00071e50;

int firewall_region_config(unsigned int rgn_id, unsigned long rgn_start, long rgn_size)
{
    unsigned int rgn_id_bit;
    long firewall_ddr_base;
    long firewall_dsu_base_paddr;
    unsigned long uVar5;
    int i;

    if (rgn_id >= FIREWALL_DDR_RGN_CNT) {
        return -1;
    }
    // Firewall DDR
    firewall_ddr_base = phys_to_virt(0xfe030000);
    // Firewall DSUDDR
    firewall_dsu_base_paddr = phys_to_virt(0xfe010000);
    put32(((unsigned long)FIREWALL_DDR_RGN(rgn_id) + firewall_ddr_base),
        ((rgn_start + rgn_size) - 1 >> 0x14) << 0x10 | rgn_start >> 0x14);
    // >> 0x14: change address to MiB; << 0x10: high 16bit of an integer
    // this means the higher 16bit stores the end address, and the lower 16bit stores the start address
    put32(((unsigned long)FIREWALL_DDR_RGN(rgn_id) + firewall_dsu_base_paddr),
        (rgn_start << (unsigned long)(_DAT_0007cd5c & 0x1f)) >> 0x14 |
        ((rgn_start + rgn_size << (unsigned long)(_DAT_0007cd5c & 0x1f)) - 1 >> 0x14) << 0x10);
    rgn_id_bit = 1 << (unsigned long)(rgn_id & 0x1f);
    put32((firewall_dsu_base_paddr + FIREWALL_DSU_MST(0)), rgn_id_bit | get32(firewall_dsu_base_paddr + FIREWALL_DSU_MST(0)));
    put32((firewall_dsu_base_paddr + FIREWALL_DSU_MST(1)), get32(firewall_dsu_base_paddr + FIREWALL_DSU_MST(1)) & (rgn_id_bit ^ 0xffffffff));
    put32((firewall_ddr_base + FIREWALL_DDR_CON), rgn_id_bit | get32(firewall_ddr_base + FIREWALL_DDR_CON));
    if (_DAT_0007cd58 == 0) {
        uVar5 = 0;
        if (_DAT_00071e50 != 0) {
            uVar5 = (rgn_start >> 0x14) / _DAT_00071e50;
        }
        firewall_ddr_base = (uVar5 + 0x3c) * 4;
        put32((firewall_ddr_base + firewall_dsu_base_paddr), rgn_id_bit | get32(firewall_ddr_base + firewall_dsu_base_paddr));
    } else {
        for (i = 0; i != _DAT_0007cd60; i++) {
            uVar5 = (unsigned long)FIREWALL_DSU_CON(i);
            put32((uVar5 + firewall_dsu_base_paddr), rgn_id_bit | get32(uVar5 + firewall_dsu_base_paddr));
        }
    }
    return 0;
}

// static void FUN_0000dd34(void)
static void secure_region_init(void)
{
    long firewall_ddr_base_paddr;
    unsigned long uVar2;
    unsigned int uVar3;

    // CRU_NS
    // long _DAT_00071e48 = phys_to_virt(0xfd7c0000);
    // SEC_SCRU
    // long _DAT_00071e68 = phys_to_virt(0xfd7d0000);
    // SYS_GRF
    // long _DAT_00071e58 = phys_to_virt(0xfd58c000);
    // BUS_SGRF
    long bussgrf_base_paddr = phys_to_virt(0xfd586000);
    // PMU1_GRF
    long pmu1grf_base_paddr = phys_to_virt(0xfd58a000);
    // CENTER_GRF
    long centergrf_base_paddr = phys_to_virt(0xfd59e000);
    // Firewall DDR
    firewall_ddr_base_paddr = phys_to_virt(0xfe030000);
    if ((get32(pmu1grf_base_paddr + PMU1GRF_OS_REG(2)) >> 0x1d & 1) == 0) {
        _DAT_0007cd60 = 1;
    } else {
        _DAT_0007cd60 = 2;
        if (get32(pmu1grf_base_paddr + PMU1GRF_OS_REG(4)) != 0) {
            _DAT_0007cd60 = 4;
        }
    }
    _DAT_0007cd58 = get32(bussgrf_base_paddr + 0x300) & 1;
    if (_DAT_0007cd58 == 0) {
        uVar2 = (unsigned long)(get32(centergrf_base_paddr + CENTER_GRF_CON(4)) >> 4) & 0x1f;
        uVar3 = 0;
        if ((unsigned int)uVar2 < 9) {
            uVar3 = *(unsigned int*)(&DAT_0005f74c + uVar2 * 4);
        }
        _DAT_00071e50 = (unsigned long)uVar3;
        _DAT_0007cd5c = 2;
    } else {
        _DAT_0007cd5c = (unsigned int)(_DAT_0007cd60 == 2);
    }

    firewall_region_config(1,0x08400000,/*0xe00000*//*0x4000000*/0x07C00000);
    // firewall_region_config(2,0x400000000UL,/*0xFFF00000UL*/0x04000000UL);
    firewall_region_config(2,0x20000000,0x50000000-0x20000000);
    firewall_region_config(3,0x02e00000,0x08000000-0x02e00000);
    // firewall_region_config(4,0x60000000,0xA0000000-0x60000000);
    firewall_region_config(4,0x60000000,0xC0000000-0x60000000);
    firewall_region_config(5,0x400000000,0x700000000-0x400000000);

    // firewall_region_config(6,0x100000000,0x190000000-0x100000000);

    put32((firewall_ddr_base_paddr + FIREWALL_DDR_MST(14)), get32(firewall_ddr_base_paddr + FIREWALL_DDR_MST(14)) & 0xfffffffd);// it seems that some devices are configured here
    put32((firewall_ddr_base_paddr + FIREWALL_DDR_MST(19)), get32(firewall_ddr_base_paddr + FIREWALL_DDR_MST(19)) & 0xfffffffd);
    return;
}

void parse_mem_map(void *info)
{
    extern char img_end;

    secure_region_init();

    physmem_map_num = 5;
#ifdef HIGH_SECURE_DEBUG
    physmem_map_num = 4;
#endif
    physmem_map[0][0] = ROUND_UP((paddr_t)&img_end, PAGE_SIZE);
    physmem_map[0][1] = ROUND_DOWN(0x08400000+0x07C00000, PAGE_SIZE);
    // physmem_map[1][0] = ROUND_UP(0x140000000UL, PAGE_SIZE);
    // physmem_map[1][1] = ROUND_DOWN(0x180000000UL/*0x400000000UL+0x04000000UL*/, PAGE_SIZE);
    physmem_map[1][0] = ROUND_UP(0x02e00000, PAGE_SIZE);
    physmem_map[1][1] = ROUND_DOWN(0x08000000, PAGE_SIZE);
    physmem_map[2][0] = ROUND_UP(0x60000000, PAGE_SIZE);
    physmem_map[2][1] = ROUND_DOWN(0xC0000000, PAGE_SIZE);
    physmem_map[3][0] = ROUND_UP(0x400000000, PAGE_SIZE);
    physmem_map[3][1] = ROUND_DOWN(0x700000000, PAGE_SIZE);
#ifndef HIGH_SECURE_DEBUG
    // this region is reserved for HIGH_SECURE_DEBUG, so shouldn't be allocated
    physmem_map[4][0] = ROUND_UP(0x20000000, PAGE_SIZE);
    physmem_map[4][1] = ROUND_DOWN(0x50000000, PAGE_SIZE);
#endif

    kinfo("[ChCore] zzh: get_tzdram_end returns 0x%lx\n", get_tzdram_end());
    // kinfo("[ChCore] physmem_map: [0x%lx, 0x%lx)\n",
        //  physmem_map[0][0], physmem_map[0][1]);
    kinfo("[ChCore] physmem_map: [0x%lx, 0x%lx), [0x%lx, 0x%lx)\n",
         physmem_map[0][0], physmem_map[0][1], physmem_map[1][0], physmem_map[1][1]);

    struct tag_tos_mem tag_tos_mem;
    memset(&tag_tos_mem, 0, sizeof(tag_tos_mem));
    memcpy(tag_tos_mem.tee_mem.name, "tee.mem", 8);
    tag_tos_mem.tee_mem.phy_addr = 0x08400000UL/*0x100000000UL*/;
    tag_tos_mem.tee_mem.size = 0x07C00000UL/*0xFFF00000UL*/;
    tag_tos_mem.tee_mem.flags = 1;
    memcpy(tag_tos_mem.drm_mem.name, "drm.mem", 8);
    tag_tos_mem.drm_mem.phy_addr = 0;
    tag_tos_mem.drm_mem.size = 0;
    tag_tos_mem.drm_mem.flags = 0;
    tag_tos_mem.version = 65536;
    set_tos_mem_tag(&tag_tos_mem);
    kinfo("zzh: set_tos_mem_tag ok!\n");

    extern void firewall_ddr_cma_rgn_init(void);
    firewall_ddr_cma_rgn_init();
}
