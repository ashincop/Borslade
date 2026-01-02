.global _start
.extern main

_start:
    xor %rbp, %rbp        # frame pointer = 0 (stack trace end)
    call _init
    mov %rsp, %rdi        # fake argc = 0
    xor %rsi, %rsi        # argv = NULL
    call main

    mov %rax, %rdi        # return value
    call exit
