/* Set the entry point to the _start symbol defined in crt0.s */
ENTRY(_start)

SECTIONS
{
    /* * Base address: Usually 0x400000 for x86_64, but change this 
     * to match where your kernel's elf-loader maps programs.
     */
    . = 0x400000;

    /* Code section */
    .text : {
        *(.text)
        *(.text.*)
    }

    /* Read-only data (strings, constants, TTF table tags) */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    /* Move to the next page to separate code from data (optional but safer) */
    . = ALIGN(4096);

    /* Initialized data */
    .data : {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data (cleared to zero) */
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(16);
    }

    /* * The Heap Boundary:
     * This symbol is used by your _sbrk stub.
     * Everything after this address is free memory for the font parser.
     */
    _end = .;
}