#pragma once
#include <stdint.h>
struct gdtr {
	uint16_t size;	 // size of GDT (sizeof) - 1
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
struct tss_entry {
	uint32_t reserved0;
	uint64_t rsp0; // <--- THE MOST IMPORTANT: Kernel Stack for Ring 0
	uint64_t rsp1; // Stack for Ring 1 (unused)
	uint64_t rsp2; // Stack for Ring 2 (unused)
	uint64_t reserved1;
	uint64_t ist[7]; // Interrupt Stack Table (for special cases like Double
			 // Faults)
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
void install_gdt();
void update_tss_rsp0(uint64_t new_rsp);