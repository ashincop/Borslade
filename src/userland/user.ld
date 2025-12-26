ENTRY(_start)

SECTIONS
{
    /* Load address for the user application */
    . = 0x400000;

    /* Executable code */
    .text : {
        *(.text .text.*)
    }

    /* Logic for _init (Constructors) */
    .init : {
        KEEP(*(SORT_NONE(.init)))
    }

    /* Logic for _fini (Destructors) */
    .fini : {
        KEEP(*(SORT_NONE(.fini)))
    }

    /* Read-only data like strings */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* Initialized global variables */
    .data : {
        *(.data .data.*)
    }

    /* Uninitialized variables (BSS) */
    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* Drop metadata that isn't needed for your kernel loader */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}