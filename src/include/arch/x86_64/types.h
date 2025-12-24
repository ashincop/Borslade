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
struct idtr {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));
struct InterruptDescriptor64 {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
   uint32_t offset_3;        // offset bits 32..63
   uint32_t zero;            // reserved
} __attribute__((packed));
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;      // <-- THIS IS YOUR FRAMEBUFFER START
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type_fb;
    uint16_t reserved;
};
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;      // <--- THE MOST IMPORTANT: Kernel Stack for Ring 0
    uint64_t rsp1;      // Stack for Ring 1 (unused)
    uint64_t rsp2;      // Stack for Ring 2 (unused)
    uint64_t reserved1;
    uint64_t ist[7];    // Interrupt Stack Table (for special cases like Double Faults)
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));
struct tss_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle_low;
    uint8_t access_byte;
    uint8_t lh_flags;
    uint8_t base_middle_high;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));