[BITS 64]

global _start
extern c_entry

_start:
    ; Optional: Set up a local stack if you aren't sure where the kernel's stack is
    ; mov rsp, stack_top 

    call c_entry    ; Jump to your C code
    
    jmp $           ; Hang if C returns

section .data
msg_asm: db "Starting Assembly Stub...", 10, 0