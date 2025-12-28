[bits 64]
extern idtc
global idtstub
; Port Constants
PIC1_COMMAND equ 0x20
PIC1_DATA    equ 0x21
PIC2_COMMAND equ 0xA0
PIC2_DATA    equ 0xA1
; Send this at the end of every IRQ handler
send_eoi:
    ; Check if the interrupt came from the slave (Int 0x28 or higher)
    ; Assuming int_no is passed in RDI
    cmp rdi, 0x28
    jl .master_only
    mov al, 0x20
    out PIC2_COMMAND, al
.master_only:
    mov al, 0x20
    out PIC1_COMMAND, al
    ret
global pic_remap
pic_remap:
    ; 1. ICW1: Start initialization in cascade mode
    mov al, 0x11
    out PIC1_COMMAND, al
    call io_wait
    out PIC2_COMMAND, al
    call io_wait

    ; 2. ICW2: Vector Offsets (Remapping IRQs)
    mov al, 0x20        ; Master offset: 0x20 (Interrupts 32-37)
    out PIC1_DATA, al
    call io_wait
    mov al, 0x28        ; Slave offset: 0x28 (Interrupts 40-47)
    out PIC2_DATA, al
    call io_wait

    ; 3. ICW3: Cascade Identity
    mov al, 0x04        ; Tell Master that Slave is at IRQ2 (0000 0100b)
    out PIC1_DATA, al
    call io_wait
    mov al, 0x02        ; Tell Slave its cascade identity (0000 0010b)
    out PIC2_DATA, al
    call io_wait

    ; 4. ICW4: Set 8086 mode
    mov al, 0x01
    out PIC1_DATA, al
    call io_wait
    out PIC2_DATA, al
    call io_wait

    ; 5. MASKS: Enable Timer(0), Keyboard(1), Cascade(2), and Mouse(12)
    ; Master: 11111000b (0xF8) -> Enables IRQ 0, 1, and 2
    mov al, 0xF8
    out PIC1_DATA, al
    
    ; Slave: 11101111b (0xEF) -> Enables IRQ 12 (Mouse)
    mov al, 0xEF
    out PIC2_DATA, al
    ret

io_wait:
    ; Port 0x80 is used for checkpoints during POST. 
    ; Writing to it is a safe way to waste a few IO cycles.
    xor al, al
    out 0x80, al
    ret
%macro ISR_NOERR 1
global isr%1
isr%1:
    push %1          ; int_no
    jmp idtstub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push %1          ; int_no (error code already pushed by CPU)
    jmp idtstub
%endmacro

; Generate all ISR stubs
ISR_NOERR 0   ; #DE Divide Error
ISR_NOERR 1   ; #DB Debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; #BP Breakpoint
ISR_NOERR 4   ; #OF Overflow
ISR_NOERR 5   ; #BR Bound Range Exceeded
ISR_NOERR 6   ; #UD Invalid Opcode
ISR_NOERR 7   ; #NM Device Not Available
ISR_ERR   8   ; #DF Double Fault
ISR_NOERR 9   ; Coprocessor Segment Overrun
ISR_ERR  10   ; #TS Invalid TSS
ISR_ERR  11   ; #NP Segment Not Present
ISR_ERR  12   ; #SS Stack Segment Fault
ISR_ERR  13   ; #GP General Protection Fault ← YOUR CRASH!
ISR_ERR  14   ; #PF Page Fault
ISR_ERR  15   ; (Intel reserved)
ISR_NOERR 16  ; #MF x87 FPU Floating-Point Error
ISR_ERR  17   ; #AC Alignment Check
ISR_NOERR 18  ; #MC Machine Check
ISR_NOERR 19  ; #SX SIMD Floating-Point Exception
ISR_NOERR 20  ; #VE Virtualization Exception
ISR_NOERR 21  ; #CP Control Protection Exception
ISR_NOERR 22  ; (Intel reserved)
ISR_NOERR 23  ; (Intel reserved)
ISR_NOERR 24  ; (Intel reserved)
ISR_NOERR 25  ; (Intel reserved)
ISR_NOERR 26  ; (Intel reserved)
ISR_NOERR 27  ; (Intel reserved)
ISR_NOERR 28  ; (Intel reserved)
ISR_NOERR 29  ; (Intel reserved)
ISR_NOERR 30  ; (Intel reserved)
ISR_NOERR 31  ; (Intel reserved)
idtstub:
    ; The CPU pushed SS, RSP, RFLAGS, CS, RIP (hardware frame)
    mov rbp, rsp
    ; Save a full register set in a consistent order for the C handler
    PUSH_ALL
    ; Pass pointer to the saved frame (rbp) as first argument
    mov rdi, rbp
    call idtc
    ; Restore registers in reverse order
    POP_ALL
    pop rbp
    iretq
global idtstubs
extern idtcs
idtstubs:
    push r11
    push r10

; NASM helper macros to keep stubs consistent
%macro PUSH_ALL 0
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
%endmacro

%macro POP_ALL 0
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
%endmacro
    push r9
    push r8
    push rsi
    push rdi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
    mov rbp, rsp
    mov rdi, rbp    ; Pointer to the struct
    
    call idtcs
    add rsp, 8          ; <--- THE FIX: "Pop" RAX into nowhere
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
    iretq           ; CPU pops RIP, CS, RFLAGS, RSP, SS
global irq0
extern current_task
extern schedule_preemptive

global irq0
irq0:
    ; --- 1. Save Task A's State ---
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

    ; --- 2. Setup RBP Frame and Argument ---
    mov rbp, rsp        ; RBP points to the saved register struct
    mov rdi, rbp        ; RDI = first argument for schedule_preemptive

    ; --- 3. Save current stack pointer into Task A's struct ---
    mov rax, [current_task]
    mov [rax], rsp      ; taskA->stack_ptr = rsp

    ; --- 4. Call the C Scheduler ---
    ; This function will swap the 'current_task' pointer to Task B
    call schedule_preemptive

    ; --- 5. Switch to Task B's Stack ---
    mov rax, [current_task]
    mov rsp, [rax]      ; rsp = taskB->stack_ptr (now points to B's stack!)

    ; --- 6. Send EOI to PIC (End of Interrupt) ---
    mov al, 0x20
    out 0x20, al

    ; --- 7. Restore Task B's State ---
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

    iretq               ; Pop the IRETQ frame and jump to Task B's RIP
global irq1
extern irq1c
irq1:
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
    mov rbp, rsp
    mov rdi, rbp    ; Pointer to the struct
    
    call irq1c

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
    call send_eoi
    iretq           ; CPU pops RIP, CS, RFLAGS, RSP, SS
global irq12
extern irq12c
irq12:
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
    mov rbp, rsp
    mov rdi, rbp    ; Pointer to the struct
    
    call irq12c

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
    call send_eoi
    iretq           ; CPU pops RIP, CS, RFLAGS, RSP, SS