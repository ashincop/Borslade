[bits 64]
extern idtc
global idtstub

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

    call idtcs

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