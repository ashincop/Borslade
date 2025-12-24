#include "idt.h"
#include <drivers/screen/fb.h>
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
void idtc() {
    kprintf("Early boot error encountered. Please seek help from Strawberry Care.\n");
    for(;;);
}
void idtcs() {
    kprintf("Say hi to the camera\n");
}
extern void idtstub();
extern void idtstubs();
void idt_install() {
    for (uint8_t i=0; i<31; i++) {
        idt_set_gate(i,(uint64_t)&idtstub, 0x8E, 0x08);
    }
    idt_set_gate(48, (uint64_t)&idtstubs, 0xEE, 0x08);
    struct idtr idtp;
    idtp.offset = (uint64_t)&idt;
    idtp.size = sizeof(idt)-1;
    __asm__ volatile ("lidt %0" : : "m"(idtp));
}