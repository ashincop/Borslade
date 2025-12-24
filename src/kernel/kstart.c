#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include "drivers/screen/fb.h"
uint8_t user_stack[(4096*9)];
extern void jump_to_user(uint64_t addr, uint64_t top);
// Define this globally so it has a fixed address in .rodata
const char* test_msg = "r3 initialized.";

void ring3() {
    asm volatile(
        "int $0x30"
        :
        : "a" (1), "b" (0x0000FF)  // No ampersand! Pass the pointer value itself.
        : "memory"
    );
    asm volatile(
        "int $0x30"
        :
        : "a" (0), "b" (test_msg)  // No ampersand! Pass the pointer value itself.
        : "memory"
    );
    for(;;);
}
void start_kernel(uint64_t mbi_addr) {
    install_gdt();
    idt_install();
    init_gop(mbi_addr);
    jump_to_user((uint64_t)ring3, (uint64_t)&user_stack);
    for(;;);
}