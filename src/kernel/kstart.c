#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include "drivers/screen/fb.h"
uint8_t user_stack[4096];
extern void jump_to_user(uint64_t addr, uint64_t top);
void ring3() {
    __asm__ volatile ("int $0x30");
    for(;;);
}
void start_kernel(uint64_t mbi_addr) {
    install_gdt();
    idt_install();
    init_gop(mbi_addr);
    jump_to_user((uint64_t)ring3, (uint64_t)&user_stack);
    for(;;);
}