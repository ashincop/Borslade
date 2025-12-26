#include "alloc.h"
#include <stddef.h>
#include <drivers/screen/fb.h>
#define PAGE_SIZE 4096
#define HEAP_MAGIC 0xCAFEBABE
#define ALIGN(size) (((size) + 7) & ~7)
uint8_t* bitmap;
uint64_t total_pages;
extern uint8_t _kernel_end;
malloc_header_t* heap_start = NULL;

// --- Physical Memory Manager (PMM) ---

void pmm_mark_used(uint64_t addr) {
    if (total_pages == 0) {
        kprintf("ERROR: total_pages is 0!\n");
        for(;;);
    }
    if (PAGE_SIZE == 0) {
        kprintf("ERROR: PAGE_SIZE is 0!\n");  
        for(;;);
    }
    uint64_t page = addr / PAGE_SIZE;
    bitmap[page / 8] |= (1 << (page % 8));
}
void pmm_mark_used64(uint64_t addr) {
    uint64_t page = addr / PAGE_SIZE;
    // Use 1ULL to ensure 64-bit operation and prevent unexpected truncation
    bitmap[page / 8] |= (1ULL << (page % 8));
}
void* pmm_alloc_page() {
    // i and j must be treated as 64-bit to prevent 32-bit wrap-around
    for (uint64_t i = 0; i < total_pages / 8; i++) {
        if (bitmap[i] != 0xFF) {
            for (uint64_t j = 0; j < 8; j++) {
                if (!(bitmap[i] & (1ULL << j))) {
                    // FORCE 64-bit math to prevent the 16TB overflow
                    uint64_t addr = (i * 8ULL + j) * (uint64_t)PAGE_SIZE;
                    pmm_mark_used(addr);
                    return (void*)addr;
                }
            }
        }
    }
    return NULL; 
}
void* pmm_alloc_pages(uint64_t count) {
    for (uint64_t i = 0; i < total_pages - count; i++) {
        int found = 1;
        for (uint64_t j = 0; j < count; j++) {
            uint64_t page = i + j;
            if (bitmap[page / 8] & (1 << (page % 8))) {
                found = 0;
                break;
            }
        }
        if (found) {
            void* addr = (void*)(i * PAGE_SIZE);
            for (uint64_t j = 0; j < count; j++) pmm_mark_used((uint64_t)addr + (j * PAGE_SIZE));
            return addr;
        }
    }
    return NULL;
}
// --- Initialization ---

void init_alloc(uint64_t mem_size_in_bytes, uint64_t mb_addr) {
    // 1. Ensure we aren't using the Multiboot Magic Number (0xE85250D6) as an address
    if (mb_addr == 0xE85250D6 || mb_addr < 0x1000) {
        // If we reach here, the arguments in start_kernel were swapped!
        return; 
    }

    // 2. Align kernel_end to page boundary
    uintptr_t kernel_end_ptr = ((uintptr_t)&_kernel_end + 4095) & ~4095;
    
    total_pages = mem_size_in_bytes / PAGE_SIZE;
    uint64_t bitmap_size = (total_pages + 7) / 8;

    // 3. Place bitmap safely after the kernel
    bitmap = (uint8_t*)kernel_end_ptr;

    // 4. Initialize bitmap
    for(uint64_t i = 0; i < bitmap_size; i++) bitmap[i] = 0;

    // 5. Reserve Kernel + Bitmap area
    uint64_t reserved_end = (uint64_t)bitmap + bitmap_size;
    for(uint64_t addr = 0; addr < reserved_end; addr += PAGE_SIZE) {
        pmm_mark_used(addr);
    }

    // 6. Reserve Multiboot Info area so kmalloc doesn't overwrite it
    uint32_t mb_info_size = *(uint32_t*)mb_addr;
    for(uint64_t addr = mb_addr; addr < mb_addr + mb_info_size; addr += PAGE_SIZE) {
        pmm_mark_used(addr);
    }

    // 7. Setup Heap (Aligned to next page)
    uint64_t heap_phys = (reserved_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    pmm_mark_used(heap_phys);

    heap_start = (malloc_header_t*)heap_phys;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = PAGE_SIZE - sizeof(malloc_header_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
}
void split_block(malloc_header_t* block, uint64_t size) {
    size = ALIGN(size);
    
    // Only split if the leftover space is large enough to hold a header + some data
    uint64_t min_split_size = sizeof(malloc_header_t) + 16;
    
    if (block->size > size + min_split_size) {
        malloc_header_t* new_block = (malloc_header_t*)((uint8_t*)(block + 1) + size);
        
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size - sizeof(malloc_header_t);
        new_block->is_free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}
// --- K-Malloc & K-Free ---
void* kmalloc(uint64_t size) {
    if (size == 0) return NULL;
    
    // 1. Align the requested size to 8 bytes for CPU efficiency
    size = ALIGN(size);

    asm volatile("cli"); // Prevent interrupts during memory manipulation

    malloc_header_t* curr = heap_start;
    malloc_header_t* last = NULL;

    // 2. SEARCH: Try to find a block in the existing heap list
    while (curr) {
        if (curr->magic != HEAP_MAGIC) {
            // If magic is wrong here, the heap was already corrupted elsewhere
            asm volatile("sti");
            return NULL; 
        }

        if (curr->is_free && curr->size >= size) {
            split_block(curr, size);
            curr->is_free = 0;
            asm volatile("sti");
            return (void*)(curr + 1);
        }
        last = curr;
        curr = curr->next;
    }

    // 3. GROW: No existing block fits. Calculate pages needed.
    // We need: Header + Requested Size
    uint64_t total_needed = size + sizeof(malloc_header_t);
    uint64_t pages_to_alloc = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;

    void* new_mem = pmm_alloc_pages(pages_to_alloc);
    if (!new_mem) {
        asm volatile("sti");
        return NULL; // System is out of RAM
    }

    // 4. INITIALIZE: Set up the new block header at the start of the new pages
    malloc_header_t* new_block = (malloc_header_t*)new_mem;
    new_block->magic = HEAP_MAGIC;
    new_block->size = (pages_to_alloc * PAGE_SIZE) - sizeof(malloc_header_t);
    new_block->is_free = 0; 
    new_block->next = NULL;

    // Link this new block into the heap chain
    if (last) {
        last->next = new_block;
    } else {
        heap_start = new_block;
    }

    // 5. SPLIT: If we allocated 2 pages for an 5KB request, 
    // split the remainder so it can be used by other kmallocs later.
    split_block(new_block, size);

    asm volatile("sti");
    return (void*)(new_block + 1);
}
void kfree(void* ptr) {
    if (!ptr) return;

    asm volatile("cli");
    malloc_header_t* header = (malloc_header_t*)ptr - 1;

    // Safety Check
    if (header->magic != HEAP_MAGIC) {
        asm volatile("sti");
        return; 
    }

    header->is_free = 1;

    // Coalesce Logic
    malloc_header_t* curr = heap_start;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            // Only merge if they are physically touching in memory
            uint8_t* curr_end = (uint8_t*)(curr + 1) + curr->size;
            if (curr_end == (uint8_t*)curr->next) {
                curr->size += sizeof(malloc_header_t) + curr->next->size;
                curr->next = curr->next->next;
                continue; // Check again for multiple merges
            }
        }
        curr = curr->next;
    }
    asm volatile("sti");
}