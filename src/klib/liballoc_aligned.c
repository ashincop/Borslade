#include <config.h>
#include <drivers/screen/fb.h>
#include <klib/liballoc_base.h>
#include <stddef.h>
#include <utils/logging/log.h>

#ifndef CONFIG_ALIGNED_ALLOC
#error "Kernel requires aligned allocation for PCIe, which is builtin."
#endif
#ifdef CONFIG_ALIGNED_ALLOC
/**
 * @brief Allocates memory with a specific alignment requirement.
 * @param alignment Must be a power of two.
 * @param size The number of bytes to allocate.
 * @return A pointer to the aligned memory block, or NULL on failure.
 */
void *kmalloc_aligned(size_t alignment, size_t size)
{
	if (alignment == 0)
		return kmalloc(size);

	// Ensure alignment is a power of 2 (common requirement for
	// hardware/MMU)
	if ((alignment & (alignment - 1)) != 0)
		return NULL;

	// We need extra space to:
	// 1. Fit the requested size
	// 2. Pad for the alignment (at most alignment - 1)
	// 3. Store a pointer to the original kmalloc'd address so we can free
	// it later
	size_t total_size = size + alignment + sizeof(void *);

	void *raw_ptr = kmalloc(total_size);
	if (!raw_ptr)
		return NULL;

	// Calculate the aligned pointer
	// We start at raw_ptr + sizeof(void*) to ensure there's room for the
	// 'back-pointer'
	uintptr_t addr = (uintptr_t)raw_ptr + sizeof(void *);
	uintptr_t aligned_addr = (addr + (alignment - 1)) & ~(alignment - 1);

	// Store the original pointer immediately before the aligned address
	// This is crucial for kfree_aligned
	((void **)aligned_addr)[-1] = raw_ptr;

	return (void *)aligned_addr;
}

/**
 * @brief Frees memory allocated by kmalloc_aligned.
 */
void kfree_aligned(void *ptr)
{
	if (!ptr)
		return;

	// Retrieve the original pointer stored just before the aligned address
	void *raw_ptr = ((void **)ptr)[-1];

	// Use your existing kfree on the original pointer
	kfree(raw_ptr);
}
#endif