#include <config.h>

OUTPUT_ARCH(powerpc)

MEMORY {
    code (rx): org = CONFIG_SYS_TEXT_BASE, len = 16m    
    data (rw): org = CONFIG_SYS_INIT_RAM_ADDR, len = 16m    
}

SECTIONS
{
    . = CONFIG_SYS_TEXT_BASE;
    __image_copy_start = .;
    .text :
    {
        KEEP(*(.text.start))
        *(.text)
        _GOT2_TABLE_ = .;
        KEEP(*(.got2))
        KEEP(*(.got))
        . = ALIGN(4);
    } > code AT> code

    .data : {
            *(.data*)
	        *(.rodata*)
            *(.sdata*)
            KEEP(*(SORT(.u_boot_list*)));
    } > data AT> code
    . = ALIGN(4);
   	__init_end = .;
    __image_copy_end = .;

    __init_start = LOADADDR(.data);
    __init_size = SIZEOF(.data);

    .bss :
    {
        PROVIDE(__bss_start = .);
        *(.bss*)
        *(.sbss*)
         PROVIDE(__bss_end = .);
    } > data

    _image_binary_end = LOADADDR(.data);
    _image_binary_end += SIZEOF(.data); 
    _end = _image_binary_end; 

}
