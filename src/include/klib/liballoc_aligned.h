#pragma once
#include <stdint.h>
#include <stddef.h>
void *kmalloc_aligned(size_t alignment, size_t size);
void kfree_aligned(void *ptr);