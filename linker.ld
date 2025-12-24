ENTRY(start)                /* The name of your entry point in boot.asm */

SECTIONS
{
    /* The bootloader will load the kernel at 1MiB physical address */
    . = 1M;

    /* Multiboot Header MUST be at the beginning of the file */
    .boot : {
        KEEP(*(.multiboot_header))
    }

    /* Executable Code */
    .text : {
        *(.text)
    }

    /* Read-only data (constants, strings, your GDT) */
    .rodata : {
        *(.rodata)
    }

    /* Initialized data */
    .data : {
        *(.data)
    }

    /* Uninitialized data (Stack, BSS) */
    .bss : {
        *(COMMON)
        *(.bss)
    }

    /* Optional: Remove unnecessary sections that could break the header placement */
    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
    }
}