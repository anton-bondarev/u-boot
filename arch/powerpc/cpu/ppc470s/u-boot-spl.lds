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
        *(.sbss*)
         PROVIDE(__bss_end = .);
    } > im0

    .data : {
            *(.data*)
	    *(.rodata*)
            *(.sdata*)
            _GOT2_TABLE_ = .;
            KEEP(*(.got2))
            KEEP(*(.got))
    } > im0
    __got2_entries = ((_GLOBAL_OFFSET_TABLE_ - _GOT2_TABLE_) >> 2) - 1;


    . = ALIGN(4);
    .u_boot_list : {
            KEEP(*(SORT(.u_boot_list*)));
    } >im0


    . = ALIGN(4);
    __image_copy_end = .;

    .end :
    {
        *(.__end)
    } > im0

    _image_binary_end = .;
    _end = .;

}
ENTRY(_start)
