[bits 64]
global load_gdt
global flush_tss
flush_tss:
    mov ax, 0x28      ; Assuming TSS is at offset 40 (index 5 * 8)
    ltr ax            ; Load Task Register
    ret
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
global jump_to_user
jump_to_user:
    ; Save the target address and stack in registers
    ; rdi = function to run, rsi = user stack top
    mov rbx, rdi 
    mov rax, rsi

    ; Clean up segment registers
    mov cx, 0x23
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx

    ; Prepare the fake stack frame
    push 0x23           ; SS
    push rax            ; RSP
    
    pushfq              ; RFLAGS
    pop rax
    or rax, 0x200       ; Enable Interrupts (Bit 9)
    push rax
    
    push 0x1B           ; CS
    push rbx            ; RIP

    ; The Point of No Return
    iretq