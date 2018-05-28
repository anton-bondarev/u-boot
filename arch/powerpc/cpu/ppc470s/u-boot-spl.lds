#include <config.h>

OUTPUT_ARCH(powerpc)
SECTIONS
{
    . = CONFIG_SPL_TEXT_BASE;
    .text :
    {
       KEEP(*(.text.start))
    *(.text)
    }

    .bss :
    {
        PROVIDE(__bss_start = .);
        *(.bss*)
        *(.sbss*)
         PROVIDE(__bss_end = .);
    }

    .data : {
            *(.data*)
	    *(.rodata*)
            *(.sdata*)
            _GOT2_TABLE_ = .;
            KEEP(*(.got2))
            KEEP(*(.got))
    }
    __got2_entries = ((_GLOBAL_OFFSET_TABLE_ - _GOT2_TABLE_) >> 2) - 1;


    . = ALIGN(4);
    .u_boot_list : {
          KEEP(*(SORT(.u_boot_list*)));
    }

    . = ALIGN(4);
	__init_end = .;    
    __image_copy_end = .;

    .end :
    {
        *(.__end)
    }

    _image_binary_end = .;
    _end = .;

}

ENTRY(_start)

ASSERT(_end <= MPW_SPL_ADDR_LIMIT, "SPL size too big");
