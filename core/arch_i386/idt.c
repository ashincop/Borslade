#include "idt.h"
void split_address(void* addr, uint16_t* high_part, uint16_t* low_part) {
    // 1. Cast the pointer to a 32-bit unsigned integer
    uint32_t full_address = (uint32_t)addr;

    // 2. Isolate the low 16 bits (masking with 0xFFFF)
    *low_part = (uint16_t)(full_address & 0xFFFF);

    // 3. Isolate the high 16 bits (right shift by 16 bits and mask)
    *high_part = (uint16_t)((full_address >> 16) & 0xFFFF);
}

void load_idt (struct idtr_pointer *idtr_p) {
    __asm__ ("lidt %0" :: "m"(*idtr_p));
} 

