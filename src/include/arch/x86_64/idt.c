#include "idt.h"
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
struct InterruptDescriptor64 idt[256];
void split_64_to_16_16_32(uint64_t input, uint16_t *low, uint16_t *mid, uint32_t *high) {
    // Extract bottom 16 bits (0-15)
    *low  = (uint16_t)(input & 0xFFFF);
    
    // Extract middle 16 bits (16-31)
    *mid  = (uint16_t)((input >> 16) & 0xFFFF);
    
    // Extract top 32 bits (32-63)
    *high = (uint32_t)((input >> 32) & 0xFFFFFFFF);
}
void idt_set_gate(uint8_t index, uint64_t offset, uint8_t attr, uint16_t selector) {
    struct InterruptDescriptor64 entry = {0};
    uint16_t low;
    uint16_t mid;
    uint32_t high;
    split_64_to_16_16_32(offset, &low, &mid, &high);
    entry.ist = 0;
    entry.offset_1 = low;
    entry.offset_2 = mid;
    entry.offset_3 = high;
    entry.selector = selector;
    entry.type_attributes = attr;
    entry.zero = 0;
    idt[index] = entry;
}
struct Registers {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
#define PORT 0x3f8          // COM1

int init_serial() {
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (38400 baud)
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   
   // Check if hardware is actually there (Loopback test)
   outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + 0, 0xAE);    // Test data
   if(inb(PORT + 0) != 0xAE) return 1;
   
   outb(PORT + 4, 0x0F);    // Set normal operation mode
   return 0;
}
int is_transmit_empty() {
   return inb(PORT + 5) & 0x20;
}
 
void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outb(PORT, a);
}

void kprint_serial(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}
void idtc(uint64_t *stack_anchor) {
    asm volatile("cli");
    uint64_t rax = *stack_anchor;
    uint64_t rbx = stack_anchor[1];
    uint64_t rcx = stack_anchor[2];
    uint64_t rdx = stack_anchor[3];
    uint64_t rbp = stack_anchor[4];
    uint64_t rdi = stack_anchor[5];
    uint64_t rsi = stack_anchor[6];
    uint64_t r8 = stack_anchor[7];
    uint64_t r9 = stack_anchor[8];
    uint64_t r10 = stack_anchor[9];
    uint64_t r11 = stack_anchor[10];
    uint64_t rip = stack_anchor[11];
    uint64_t cs = stack_anchor[12];
    uint64_t rflags = stack_anchor[13];
    uint64_t rsp = stack_anchor[14];
    uint64_t ss = stack_anchor[15];

    clear_screen(0x0000FF);
    kprintf("\npanic(exception)\n");
    
    // 1. Check the Registers we pushed
    kprintf("RAX: %x  RBX: %x  RCX: %x\n", rax, rbx, rcx);
    kprintf("RDI: %x  RSI: %x\n", rdi, rsi);


    // 3. Check the Trap Frame (The "Ticket" back to Ring 3)
    // If these look like memory addresses instead of 0x1B/0x23, your struct is wrong!
    kprintf("RIP:        %x\n", rip);
    kprintf("CS:         %x\n", cs);
    kprintf("RFLAGS:     %x\n", rflags);
    kprintf("RSP:        %x\n", rsp);
    kprintf("SS:         %x\n", ss);

    // 4. Verify Stack Alignment
    // If (RIP address - RAX address) != 88, your C struct and ASM pushes don't match.
    uint64_t offset = (uint64_t)&rip - (uint64_t)&rax;
    kprintf("STRUCT OFFSET: %d bytes\n", (int)offset);

    kprintf("------------------------------------------\n");
    
}
void idtcs(uint64_t *stack_anchor) {
    uint64_t rax = *stack_anchor;
    uint64_t rbx = stack_anchor[1];
    uint64_t rcx = stack_anchor[2];
    uint64_t rdx = stack_anchor[3];
    uint64_t rbp = stack_anchor[4];
    uint64_t rdi = stack_anchor[5];
    uint64_t rsi = stack_anchor[6];
    uint64_t r8 = stack_anchor[7];
    uint64_t r9 = stack_anchor[8];
    uint64_t r10 = stack_anchor[9];
    uint64_t r11 = stack_anchor[10];
    uint64_t rip = stack_anchor[11];
    uint64_t cs = stack_anchor[12];
    uint64_t rflags = stack_anchor[13];
    uint64_t rsp = stack_anchor[14];
    uint64_t ss = stack_anchor[15];
    if (rax == 0) {
        kprintf("[com.strawberry.core.userland] %s", (char*)rbx);
    } else if (rax == 1) {
        clear_screen((uint32_t)rbx);
    } else if (rax == 2) {
        kprintf("[com.strawberry.core.interrupts] RAX=2.\n");
        asm volatile("sti");
        kscan((char *)rbx);
        asm volatile("cli");
    }
}
void irq0c(uint64_t *stack_anchor) {

}
void irq1c(uint64_t *stack_anchor) {
    keyboard_handler(stack_anchor);
}
void irq12c(uint64_t *stack_anchor) {

}
extern void idtstub();
extern void idtstubs();
extern void irq0();
extern void irq1();
extern void irq12();
extern void pic_remap();
void idt_install() {
    __asm__ volatile ("cli");
    for (uint8_t i=0; i<31; i++) {
        idt_set_gate(i,(uint64_t)&idtstub, 0xEE, 0x08);
    }
    idt_set_gate(48, (uint64_t)&idtstubs, 0xEE, 0x08);
    idt_set_gate(32, (uint64_t)irq0, 0xEE, 0x08);
    idt_set_gate(33, (uint64_t)irq1, 0xEE, 0x08);
    idt_set_gate(44, (uint64_t)irq12, 0xEE, 0x08);
    struct idtr idtp;
    idtp.offset = (uint64_t)&idt;
    idtp.size = sizeof(idt)-1;
    pic_remap();
    keyboard_init();
    __asm__ volatile ("lidt %0" : : "m"(idtp));
    __asm__ volatile ("sti");
}