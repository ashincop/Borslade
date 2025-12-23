#pragma once
#include <stdint.h>
#define BIG_GRANULARITY 0x1
#define SMALL_GRANULARITY 0x0
struct gdtr_pointer {
    uint16_t size;    // Bits 0-15
    uint32_t offset;  // Bits 16-47 (32-bit address)
} __attribute__((packed));

struct gdt_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_byte;
    uint8_t fl_lh;
    uint8_t base_high;
};