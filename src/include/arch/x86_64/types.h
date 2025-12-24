#pragma once
#include "stdint.h"
struct gdtr {
    uint16_t size; // size of GDT (sizeof) - 1
    uint64_t offset; // address of GDT, paging applies
} __attribute__((packed));
struct gdt_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_byte;
    uint8_t lh_flags;
    uint8_t base_high;
} __attribute__((packed));