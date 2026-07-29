
ENTRY(_start)

SECTIONS
{
    . = 0x8400000;

    img_start = .;
    init : {
        arch/aarch64/boot/rk3588/init/start.S.o arch/aarch64/boot/rk3588/init/mmu.S.o arch/aarch64/trustzone/spd/opteed/init/smc.S.o arch/aarch64/trustzone/spd/opteed/init/cpu.S.o arch/aarch64/boot/rk3588/init/page_table.c.o arch/aarch64/boot/rk3588/init/start.c.o
    }
    . = ALIGN(4K);
    init_end = ABSOLUTE(.);

    .text 0xffffff0000000000 + init_end : AT(init_end) {
    _text_start = .;
        *(.text*)
    }
    . = ALIGN(4K);
    _text_end = .;

    .data : {
        *(.data*)
    }
    . = ALIGN(4K);

    .rodata : {
        *(.rodata*)
    }
    . = ALIGN(4K);
    _edata = . - 0xffffff0000000000;

    _bss_start = . - 0xffffff0000000000;
    .bss : {
        *(.bss*)
    }
    . = ALIGN(4K);
    _bss_end = . - 0xffffff0000000000;

    _extable_start = .;
    .extable : {
        *(.extable*)
    }
    . = ALIGN(4K);
    _extable_end = .;

    img_end = . - 0xffffff0000000000;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
