#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
uint8_t user_stack[(4096*9)];
extern void jump_to_user(uint64_t addr, uint64_t top);
// Define this globally so it has a fixed address in .rodata
const char* test_msg = "r3 initialized.\n";
char buf[32];
void ring3() {
    
    const char* prompt = "\nEnter something: ";

    // 1. Clear Screen (RAX=1)
    asm volatile("int $0x30" : : "a"(1), "b"(0x0000FF));

    // 2. Print Prompt (RAX=0)
    asm volatile("int $0x30" : : "a"(0), "b"(prompt));

    // 3. Get Input (RAX=2)
    // This will block until you hit Enter
    asm volatile("int $0x30" : : "a"(2), "b"(buf) : "memory");

    // 4. Print the Result (RAX=0)
    // We pass 'buf' back to the kernel to prove it was filled
    asm volatile("int $0x30" : : "a"(0), "b"(buf));

    for(;;);
}
void start_kernel(uint64_t mbi_addr) {
    install_gdt();
    idt_install();
    char buf[32];
    init_gop(mbi_addr);
    kprintf("KM: ");
    kscan(buf);
    jump_to_user((uint64_t)ring3, (uint64_t)&user_stack);
    for(;;);
}