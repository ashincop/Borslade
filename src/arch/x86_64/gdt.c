#include <arch/x86_64/gdt.h>
#include <utils/mem/mem.h>
struct gdt_descriptor gdt[7];
struct tss_entry my_tss; // Global TSS instance
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
void update_tss_rsp0(uint64_t new_rsp) {
    // In a 64-bit TSS, rsp0 starts at byte 4 and is 8 bytes long.
    // Since my_tss is a global instance, we just overwrite that field.
    my_tss.rsp0 = new_rsp;
}
void gdt_set_tss_gate(int index, uint64_t base, uint32_t limit) {
    // We cast the GDT pointer to your 16-byte struct
    // Warning: 'index' here refers to the GDT entry number. 
    // Since this is 16 bytes, it will overwrite gdt[index] and gdt[index+1].
    struct tss_descriptor *gate = (struct tss_descriptor *)&gdt[index];

    // 1. Set the Limit (usually sizeof(tss) - 1)
    gate->limit_low = (uint16_t)(limit & 0xFFFF);
    
    // 2. Set the Base Address (Split into 4 parts)
    gate->base_low         = (uint16_t)(base & 0xFFFF);
    gate->base_middle_low  = (uint8_t)((base >> 16) & 0xFF);
    gate->base_middle_high = (uint8_t)((base >> 24) & 0xFF);
    gate->base_high        = (uint32_t)(base >> 32);

    // 3. The Access Byte (0x89)
    // 0x89 = 10001001b (Present, DPL 0, System, Type: 64-bit TSS available)
    gate->access_byte = 0x89;

    // 4. Flags and Limit High
    // 0x40 = 01000000b (Available bit set, Limit bits 16-19 are 0)
    gate->lh_flags = (uint8_t)((limit >> 16) & 0x0F);
    gate->lh_flags |= 0x40; 

    // 5. Clean up reserved
    gate->reserved = 0;
}
// Create a separate stack for the kernel to use when we're interrupted in User Mode
uint8_t kernel_stack[8192]; 

void init_tss() {
    memset(&my_tss, 0, sizeof(my_tss));
    
    // rsp0 is where the CPU jumps when an interrupt happens in Ring 3.
    // We point it to the TOP of our kernel_stack array.
    my_tss.rsp0 = (uint64_t)&kernel_stack[8192];
    
    // Some CPUs require the IOPB to point beyond the TSS limit
    my_tss.iopb_offset = sizeof(my_tss);
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
extern void flush_tss();
void install_gdt() {
    gdt_set_gate(0,0,0,0,0);
    gdt_set_gate(1,0,0,0x9B,0xA);
    gdt_set_gate(2,0,0,0x93,0xA);
    gdt_set_gate(3,0,0,0xFB,0xA);
    gdt_set_gate(4,0,0,0xF3,0xA);
    // Correct way to call it in install_gdt:
gdt_set_tss_gate(5, (uint64_t)&my_tss, sizeof(my_tss) - 1);
    struct gdtr gdtp;
    gdtp.offset = (uint64_t)&gdt;
    gdtp.size = sizeof(gdt)-1;
    load_gdt(&gdtp);
    init_tss();
    flush_tss();
}