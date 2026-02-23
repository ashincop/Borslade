#pragma once
#include <stdint.h>
typedef struct malloc_header {
	uint32_t magic;
	uint64_t size;
	int is_free;
	struct malloc_header *next;
} malloc_header_t;
void init_alloc(uint64_t mem_size_in_bytes, uint64_t mb_addr);
void *kmalloc(uint64_t size);
void *pmm_alloc_page();
void kfree(void *ptr);
void pmm_mark_used(uint64_t addr);
void pmm_mark_used64(uint64_t addr);
void *pmm_alloc_pages(uint64_t count);
// Debug helper: log page range for an address + byte length, dump nearby bitmap
// and memory
void pmm_debug_range(uint64_t addr, uint64_t bytes);