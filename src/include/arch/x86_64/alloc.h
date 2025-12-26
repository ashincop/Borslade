#pragma once
#include "types.h"
void init_alloc(uint64_t mem_size_in_bytes, uint64_t mb_addr);
void* kmalloc(uint64_t size);
void* pmm_alloc_page();
void kfree(void* ptr);
void pmm_mark_used(uint64_t addr);
void pmm_mark_used64(uint64_t addr);