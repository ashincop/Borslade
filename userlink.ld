OUTPUT_FORMAT("binary")    /* This is the magic line */
ENTRY(_start)

SECTIONS
{
    . = 0x0;              /* The location counter */
    .text : { *(.text) }  /* Code first */
    .data : { *(.data) }  /* Then data */
    .bss  : { *(.bss)  }  /* Then uninitialized data */
}