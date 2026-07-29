#include <arch/tools.h>
#include <arch/mmu.h>
#include <machine.h>
#include <common/kprint.h>

static inline void set32(vaddr_t addr, u32 set_mask)
{
    put32(addr, 0xffff0000 | (get32(addr) | set_mask));
}
static inline void clr32(vaddr_t addr, u32 set_mask)
{
    put32(addr, 0xffff0000 | (get32(addr) & ~set_mask));
}

void switch_secure_device(int secure, int core_index)
{
    vaddr_t bussgrf_vaddr = phys_to_virt(BUSSGRF_BASE);

    if (secure) {
        if (core_index == 0) set32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x3);
        if (core_index == 1) set32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x4);
        if (core_index == 2) set32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x8);
        // put32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0xffff000f);
    } else {
        if (core_index == 0) clr32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x3);
        if (core_index == 1) clr32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x4);
        if (core_index == 2) clr32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0x8);
        // put32(bussgrf_vaddr + SGRF_FIREWALL_CON(8), 0xffff0000);
    }
}
