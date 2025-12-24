; --- Constants for Multiboot2 ---
MULTIBOOT2_MAGIC        equ 0xe85250d6
MULTIBOOT2_ARCH         equ 0       ; 0 = Protected Mode i386 (required for x86/x86_64)

section .multiboot_header
header_start:
    ; Magic number
    dd MULTIBOOT2_MAGIC
    ; Architecture
    dd MULTIBOOT2_ARCH
    ; Header length
    dd header_end - header_start
    ; Checksum (must sum to 0 when added to magic, arch, and length)
    dd 0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + (header_end - header_start))

    ; --- TAGS START HERE ---
    
    ; Optional: Request Framebuffer (useful for graphics)
    align 8
    dw 5          ; Type: Framebuffer
    dw 0          ; Flags
    dd 20         ; Size
    dd 1280       ; Width
    dd 720        ; Height
    dd 32         ; Depth

    ; Required: End Tag
    align 8
    dw 0    ; Type: 0 (End)
    dw 0    ; Flags: 0
    dd 8    ; Size: 8
header_end:
extern start_kernel
call start_kernel