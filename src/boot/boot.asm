; --- Constants for Multiboot2 ---
MULTIBOOT2_MAGIC        equ 0xe85250d6
MULTIBOOT2_ARCH         equ 0

section .multiboot_header
header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd header_end - header_start
    dd 0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + (header_end - header_start))

    ; Framebuffer Tag
    align 8
    dw 5          ; Type
    dw 0          ; Flags
    dd 20         ; Size
    dd 1280       ; Width
    dd 720        ; Height
    dd 32         ; Depth

    ; End Tag
    align 8
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_tables:    ; We reserve 4 tables here (4 * 4096)
    resb 16384
stack_bottom:
    resb 16384          ; 16 KiB
stack_top:

section .text
bits 32
global start
start:
    mov esp, stack_top   

    call setup_paging
    call enable_long_mode

    ; Load the temporary 64-bit GDT for the jump
    lgdt [gdt64_ptr]

    ; Perform the Far Jump to enter 64-bit Long Mode
    ; We use the 'dword' prefix to help NASM with the 32-to-64 bit transition
    jmp 0x08:start_64_trampoline

setup_paging:
    ; 1. Link P4 to P3
    mov eax, p3_table
    or eax, 0b111        ; present + writable
    mov [p4_table], eax

    ; 2. Link P3 to 4 P2 tables (Mapping 4GB total)
    mov ecx, 0
.link_p3_to_p2:
    mov eax, p2_tables
    mov edx, 4096
    imul edx, ecx
    add eax, edx        ; EAX = p2_tables + (ecx * 4096)
    or eax, 0b111        ; present + writable
    mov [p3_table + ecx * 8], eax
    inc ecx
    cmp ecx, 4
    jne .link_p3_to_p2

    ; 3. Identity map the first 4GB using 2MB Huge Pages
    mov ecx, 0          ; Counter for 2048 entries (4 tables * 512 entries)
.map_p2_table:
    mov eax, 0x200000   ; 2MB
    mov edx, ecx
    mul edx             ; EAX = ecx * 2MB
    or eax, 0b10000111  ; present + writable + huge + user
    mov [p2_tables + ecx * 8], eax

    inc ecx
    cmp ecx, 2048       ; 2048 * 2MB = 4096MB (4GB)
    jne .map_p2_table

    ret

enable_long_mode:
    ; Pass P4 table to CR3
    mov eax, p4_table
    mov cr3, eax

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set Long Mode Enable bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

bits 64
start_64_trampoline:
    ; Reload all data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    extern start_kernel
    mov rdi, rbx
    call start_kernel
    
    ; Should never reach here
    cli
.hlt_loop:
    hlt
    jmp .hlt_loop

section .rodata
align 8
gdt64:
    dq 0 ; null
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) 
.data: equ $ - gdt64
    dq (1<<41) | (1<<44) | (1<<47)
gdt64_ptr:
    dw $ - gdt64 - 1
    dq gdt64