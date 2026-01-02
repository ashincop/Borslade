[bits 64]
section .entry
global _start
extern main

_start:
    ; BYPASS BSS/_init/_fini - Kernel memset handles zeroing!
    ; rdi=argc=1, rsi=argv from spawn_user_task - PERFECT!
    
    call main              ; main(1, ["tcc.sys"])

    ; Exit syscall (ignore return value)
    mov rax, 1
    int 0x30

.halt:
    cli
    hlt
    jmp .halt
