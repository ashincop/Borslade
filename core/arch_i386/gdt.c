#include "gdt.h"
struct gdt_descriptor gdt[3];
void split_16_4(uint32_t input, uint16_t *low_16, uint8_t *high_4) {
    // 1. Extract the bottom 16 bits (0xFFFF is 1111111111111111 in binary)
    *low_16 = (uint16_t)(input & 0xFFFF);

    // 2. Shift right by 16 to move the upper bits to the bottom, 
    // then mask with 0x0F to isolate only 4 bits.
    *high_4 = (uint8_t)((input >> 16) & 0x0F);
}
void split_32_to_16_8_8(uint32_t input, uint16_t *low_16, uint8_t *mid_8, uint8_t *high_8) {
    // 1. Get the lowest 16 bits (e.g., bits 0-15)
    *low_16 = (uint16_t)(input & 0xFFFF);

    // 2. Get the middle 8 bits (e.g., bits 16-23)
    *mid_8 = (uint8_t)((input >> 16) & 0xFF);

    // 3. Get the highest 8 bits (e.g., bits 24-31)
    *high_8 = (uint8_t)((input >> 24) & 0xFF);
}
uint8_t merge_nibbles(uint8_t high_nibble, uint8_t low_nibble) {
    // 1. (high_nibble << 4) moves 0000AAAA to AAAA0000
    // 2. (low_nibble & 0x0F) ensures no stray bits exist in the top half
    // 3. The '|' (OR) joins them into AAAABBBB
    return (uint8_t)((high_nibble << 4) | (low_nibble & 0x0F));
}
void gdt_set_gate(uint32_t index, uint32_t segment_address, uint8_t flags, uint8_t size, uint8_t access_byte) {
    uint16_t limit_low;
    uint8_t limit_high;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t base_high;
    split_16_4(size, &limit_low, &limit_high);
    split_32_to_16_8_8(segment_address, &base_low, &base_middle, &base_high);
    uint8_t fl_lh = merge_nibbles(flags, limit_high);
    struct gdt_descriptor gdtentry;
    gdtentry.limit_low = limit_low;
    gdtentry.base_low = base_low;
    gdtentry.access_byte = access_byte;
    gdtentry.fl_lh = fl_lh;
    gdtentry.base_middle = base_middle;
    gdtentry.base_high = base_high;
    gdt[index] = gdtentry;
}

void gdt_install() {
    gdt_set_gate(0,0,0,0,0);
    gdt_set_gate(1,0x0,0x3,0xFFFFF,0xD9);
    gdt_set_gate(2,0xFFFFF+1,0x3,1024*32, 0xC9);
    struct gdtr_pointer gdtp_var;
    struct gdtr_pointer *gdtp = &gdtp_var;

    gdtp->size = sizeof(gdt)-1;
    gdtp->offset = (uint32_t)&gdt;
    __asm__ volatile (
        "lgdt %0"    // Move the value 30 into the memory location designated by %0
        : "=m" (*gdtp)     // Output operand: the memory pointed to by 'ptr'
        :                 // No input operands
        :
    );
}