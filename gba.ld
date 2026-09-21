ENTRY(_start)
MEMORY {
    ROM (rx) : ORIGIN = 0x08000000, LENGTH = 32M
    IWRAM (rwx) : ORIGIN = 0x03000000, LENGTH = 28K
    EWRAM (rw) : ORIGIN = 0x02000000, LENGTH = 256K
}
SECTIONS {
    .header : { KEEP(*(.header)) } > ROM
    /* Execute the small game from fast internal RAM. */
    .text : ALIGN(4) {
        __text_start = .;
        *(.text*) *(.glue_7*) *(.v4_bx*)
        . = ALIGN(4); __text_end = .;
    } > IWRAM AT> ROM
    __text_load = LOADADDR(.text);
    .rodata : ALIGN(4) { *(.rodata*) } > ROM
    .data : ALIGN(4) {
        __data_start = .; *(.data*) . = ALIGN(4); __data_end = .;
    } > EWRAM AT> ROM
    __data_load = LOADADDR(.data);
    .bss (NOLOAD) : ALIGN(4) {
        __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(4); __bss_end = .;
    } > EWRAM
    /DISCARD/ : { *(.comment) *(.ARM.exidx*) *(.ARM.extab*) *(.eh_frame*) }
}
