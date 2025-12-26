#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/multi.h>
#include <arch/x86_64/alloc.h>
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/storage/vfs/vfs.h>
void enable_sse() {
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear EM (Coprocessor Emulation)
    cr0 |= (1 << 1);  // Set MP (Monitor Coprocessor)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // Set OSFXSR (FXSAVE/FXRSTOR support)
    cr4 |= (1 << 10); // Set OSXMMEXCPT (SIMD Exception support)
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}
uint8_t user_stack[(4096*9)];
uint64_t probe_memory_size() {
    uint64_t last_accessible_addr = 0;
    // Start probing every 1MB starting after the kernel
    for (uint64_t addr = 0x1000000; addr < 0xFFFFFFFF; addr += 0x100000) {
        volatile uint64_t* ptr = (uint64_t*)addr;
        uint64_t backup = *ptr;
        *ptr = 0xDEADBEEF;
        if (*ptr == 0xDEADBEEF) {
            *ptr = backup;
            last_accessible_addr = addr;
        } else {
            break;
        }
    }
    return last_accessible_addr;
}

void start_kernel(uint64_t mbi_addr) {
    install_gdt();
    idt_install();
    char buf[32];
    init_alloc(0x20000000, mbi_addr);
    init_gop(mbi_addr);
    enable_sse();
    init_vfs(mbi_addr);
    __asm__ volatile ("cli");
    init_multitasking();
    __asm__ volatile ("sti");
    for(;;);
}