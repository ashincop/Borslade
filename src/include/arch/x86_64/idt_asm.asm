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
idtstub:
    ; The CPU pushed SS, RSP, RFLAGS, CS, RIP (40 bytes)
    push rbp            ; +8 bytes = 48 bytes (aligned to 16!)
    mov rbp, rsp
    
    ; Save registers because C might overwrite them
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    call idtc

    ; Restore registers
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    pop rbp
    iretq
global idtstubs
extern idtcs
idtstubs:
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
    
    call idtcs

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
    iretq           ; CPU pops RIP, CS, RFLAGS, RSP, SS
global irq0
extern irq0c
irq0:
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
    
    call irq0c

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