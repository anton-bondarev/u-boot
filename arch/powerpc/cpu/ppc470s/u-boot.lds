MEMORY
{
    rom (rx) : ORIGIN = 0xFFFC0000, LENGTH = 0x00040000
    im0 (rx) : ORIGIN = 0x00040000, LENGTH = 0x00040000
    ddr (rx) : ORIGIN = 0x40000000, LENGTH = 0x20000000
}

OUTPUT_ARCH(powerpc)
SECTIONS
{
    .text :
    {
    KEEP(*(.text.start))
    *(.text)
    } > ddr

    .bss :
    {
        PROVIDE(__bss_start = .);
        *(.bss)
        *(.sbss*)
         PROVIDE(__bss_end = .);
    } > ddr

    .data :
    {
        *(.rodata*)
        *(.data*)
        *(.sdata*)
    } > ddr

    . = ALIGN(4);
    __image_copy_end = .;

    .end :
    {
        *(.__end)
    } > ddr

    _image_binary_end = .;
    _end = .;

}
ENTRY(_start)
