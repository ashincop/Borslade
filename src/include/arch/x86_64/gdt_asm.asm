[bits 64]
global load_gdt
load_gdt:
    lgdt [rdi]        ; Load the GDT pointer from the address in RDI
    
    ; Reload data segment registers
    mov ax, 0x10      ; Offset of your Kernel Data segment (usually 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload Code Segment (CS)
    ; In 64-bit mode, we use a 'Far Return' to update CS
    push 0x08         ; New CS selector (usually 0x08)
    lea rax, [rel .reload_cs]
    push rax          ; Address to "return" to
    retfq             ; Perform the far return

.reload_cs:
    ret