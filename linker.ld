ENTRY(start)

SECTIONS
{
    . = 1M; /* Start at 1 MiB */

    .text : ALIGN(4K)
    {
        *(.multiboot_header)
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
    
    /DISCARD/ : { *(.comment) *(.eh_frame) }
}