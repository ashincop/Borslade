#include "gdt.h"
struct gdt_descriptor gdt[3];
uint8_t combine_limit_flags(uint8_t flags, uint8_t limit_high) {
    // Ensure flags are in the high 4 bits and limit_high is in the low 4 bits
    return (flags << 4) | (limit_high & 0x0F);
}
void split_base_32(uint32_t base, uint16_t *low, uint8_t *mid, uint8_t *high) {
    *low  = (uint16_t)(base & 0xFFFF);          // Bits 0-15
    *mid  = (uint8_t)((base >> 16) & 0xFF);     // Bits 16-23
    *high = (uint8_t)((base >> 24) & 0xFF);     // Bits 24-31
}
void split_byte_to_nibbles(uint8_t input, uint8_t *high_nibble, uint8_t *low_nibble) {
    *high_nibble = (input >> 4) & 0x0F; // Extract Flags
    *low_nibble  = input & 0x0F;        // Extract Limit High
}
void split_limit_20(uint32_t limit, uint16_t *low, uint8_t *high) {
    // Extract the lower 16 bits (0-15)
    *low = (uint16_t)(limit & 0xFFFF);
    
    // Extract the upper 4 bits (16-19)
    *high = (uint8_t)((limit >> 16) & 0x0F);
}
extern void load_gdt(struct gdtr* gdt);
void gdt_set_gate(uint8_t index, uint32_t base, uint32_t size, uint8_t access_byte, uint8_t flags) {
    uint16_t limit_low;
    uint8_t limit_high;
    split_limit_20(size, &limit_low, &limit_high);
    uint8_t lh_flags = combine_limit_flags(flags, limit_high);
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t base_high;
    split_base_32(base, &base_low, &base_middle, &base_high);
    struct gdt_descriptor entry = {0};
    entry.lh_flags = lh_flags;
    entry.limit_low = limit_low;
    entry.base_low = base_low;
    entry.base_middle = base_middle;
    entry.base_high = base_high;
    entry.access_byte = access_byte;
    gdt[index] = entry;
}
void install_gdt() {
    gdt_set_gate(0,0,0,0,0);
    gdt_set_gate(1,0,0,0x9B,0xA);
    gdt_set_gate(2,0,0,0x93,0xA);
    struct gdtr gdtp;
    gdtp.offset = (uint64_t)&gdt;
    gdtp.size = sizeof(gdt)-1;
    load_gdt(&gdtp);
}