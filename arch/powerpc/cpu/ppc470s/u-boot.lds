MEMORY
{
    rom (rx) : ORIGIN = 0xFFFC0000, LENGTH = 0x00040000
    im0 (rx) : ORIGIN = 0x00040000, LENGTH = 0x00040000
}

OUTPUT_ARCH(powerpc)
SECTIONS
{
    .text :
    {
    KEEP(*(.text.start))
    *(.text)
    } > im0

    .bss :
    {
        PROVIDE(__bss_start = .);
        *(.bss)
        PROVIDE(__bss_end = .);
    } > im0

    .data :
    {
        *(.data)
    } > im0

    .rodata :
    {
    *(.rodata)
    *(.rodata).*
    } > im0
}
ENTRY(_start)
