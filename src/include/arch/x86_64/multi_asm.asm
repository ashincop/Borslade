; context_switch(task_t* old, task_t* new)
; rdi = pointer to old task struct
; rsi = pointer to new task struct
global read_cr3
global context_switch
context_switch:
    ; 1. Save ALL registers to match the new preemptive format
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax

    ; 2. Save the current stack pointer
    mov [rdi], rsp

    ; 3. Load the new stack pointer
    mov rsp, [rsi]

    ; 4. CR3 Switch
    mov rax, [rsi + 8]
    mov rdx, cr3
    cmp rax, rdx
    je .skip_cr3
    mov cr3, rax
.skip_cr3:

    ; 5. Pop ALL registers (Must be exactly 15 pops)
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; 6. CRITICAL CHANGE: 
    ; If the new task was set up for an interrupt (iretq), we use iretq.
    ; If we are still just testing manually, we use ret.
    ; For now, to make your manual call work, keep this as 'ret' 
    ; but ensure your C code doesn't push the 5 interrupt regs yet.
    iretq
read_cr3:
    mov rax, cr3
    ret