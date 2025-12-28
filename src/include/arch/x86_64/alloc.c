#include "alloc.h"
#include <stddef.h>
#include <drivers/screen/fb.h>

#define PAGE_SIZE 4096
#define HEAP_MAGIC 0xCAFEBABE
#define ALIGN(size) (((size) + 7) & ~7)
// Extra low-pages to reserve as a guard so dynamic allocations land higher
// Bumped from 512 to 8192 to push allocations well away from kernel/bitmap
#define PMM_RESERVED_GUARD_PAGES 8192
uint8_t* bitmap;
uint64_t total_pages;
extern uint8_t _kernel_end;
malloc_header_t* heap_start = NULL;
// Number of low pages reserved for kernel+bitmap+heap
uint64_t pmm_reserved_pages = 0;
// debug: limit verbose PMM marking prints
static uint64_t pmm_debug_marks = 0;
// When non-zero, PMM will print every mark/allocation (can be noisy)
static int pmm_verbose = 1;

// --- Physical Memory Manager (PMM) ---

void pmm_mark_used(uint64_t addr) {
    // Reuse the 64-bit checked implementation to avoid code duplication
    pmm_mark_used64(addr);
}
void pmm_mark_used64(uint64_t addr) {
    if (PAGE_SIZE == 0 || total_pages == 0) {
        kprintf("ERROR: pmm_mark_used64 called before init (PAGE_SIZE=%d total_pages=%d)\n", (int)PAGE_SIZE, (int)total_pages);
        return;
    }
    if (!bitmap) {
        kprintf("ERROR: pmm_mark_used64 bitmap is NULL\n");
        return;
    }
    uint64_t page = addr / PAGE_SIZE;
    if (page >= total_pages) {
        kprintf("ERROR: pmm_mark_used64 out-of-range addr=%p page=%d total_pages=%d\n",
                (void*)addr, (int)page, (int)total_pages);
        return;
    }

    // Compute bitmap size and check index to avoid OOB writes
    uint64_t bitmap_size = (total_pages + 7) / 8;
    uint64_t bit_index = page / 8;
    if (bit_index >= bitmap_size) {
        kprintf("PANIC: pmm_mark_used64 BITMAP OOB: page=%d bit_index=%d bitmap_size=%d bitmap=%p addr=%p\n",
            (int)page, (int)bit_index, (int)bitmap_size, bitmap, (void*)addr);
        asm volatile("cli");
        for(;;) asm volatile("hlt");
    }

    // Log marks — either all when verbose, or the first few otherwise
    if (pmm_verbose) {
        kprintf("PMM: mark page %d (addr=%p)\n", (int)page, (void*)addr);
    } else if (pmm_debug_marks < 32) {
        kprintf("PMM: mark page %d (addr=%p)\n", (int)page, (void*)addr);
        pmm_debug_marks++;
    }

    bitmap[page / 8] |= (1ULL << (page % 8));
}
void* pmm_alloc_page() {
    // iterate over the whole bitmap (round up total_pages to bytes)
    uint64_t bitmap_size = (total_pages + 7) / 8;
    kprintf("PMM: pmm_alloc_page total_pages=%d bitmap_size=%d bitmap=%p\n", (int)total_pages, (int)bitmap_size, bitmap);
    // Use precomputed reserved pages from init_alloc
    uint64_t reserved_pages = pmm_reserved_pages;
    kprintf("PMM: reserved_pages=%d\n", (int)reserved_pages);
    for (uint64_t i = 0; i < bitmap_size; i++) {
        if (bitmap[i] != 0xFF) {
            for (uint64_t j = 0; j < 8; j++) {
                uint64_t page = i * 8ULL + j;
                if (page >= total_pages) break; // don't scan past end
                if (page < reserved_pages) continue; // skip reserved low pages
                if (!(bitmap[i] & (1ULL << j))) {
                    uint64_t addr = page * (uint64_t)PAGE_SIZE;
                    kprintf("PMM: pmm_alloc_page allocating page %d addr=%p\n", (int)page, (void*)addr);
                    pmm_mark_used64(addr);
                    return (void*)addr;
                }
            }
        }
    }
    return NULL;
}
void* pmm_alloc_pages(uint64_t count) {
    if (count == 0) return NULL;
    if (count > total_pages) return NULL;
    kprintf("PMM: pmm_alloc_pages count=%d total_pages=%d\n", (int)count, (int)total_pages);

    uint64_t max_start = total_pages - count; // inclusive
    uint64_t bitmap_size = (total_pages + 7) / 8;
    uint64_t reserved_pages = pmm_reserved_pages;
    kprintf("PMM: pmm_alloc_pages reserved_pages=%d max_start=%d bitmap_size=%d\n", (int)reserved_pages, (int)max_start, (int)bitmap_size);
    for (uint64_t i = 0; i <= max_start; i++) {
        int found = 1;
        for (uint64_t j = 0; j < count; j++) {
            uint64_t page = i + j;
            uint64_t bit_index = page / 8;
            if (bit_index >= bitmap_size) {
                kprintf("PANIC: pmm_alloc_pages BITMAP OOB access: page=%d bit_index=%d bitmap_size=%d\n",
                        (int)page, (int)bit_index, (int)bitmap_size);
                asm volatile("cli");
                for(;;) asm volatile("hlt");
            }
            uint8_t byte = bitmap[bit_index];
            if (byte & (1ULL << (page % 8))) {
                found = 0;
                break;
            }
        }
        if (found) {
            if (i < reserved_pages) continue; // don't hand out pages inside reserved region
            void* addr = (void*)(i * PAGE_SIZE);
            kprintf("PMM: pmm_alloc_pages found range start_page=%d addr=%p count=%d\n", (int)i, addr, (int)count);
            for (uint64_t j = 0; j < count; j++) pmm_mark_used64((uint64_t)addr + (j * PAGE_SIZE));
            return addr;
        }
    }
    return NULL;
}

// Debug helper: show computed page range and nearby bitmap + memory dump
void pmm_debug_range(uint64_t addr, uint64_t bytes) {
    if (!bitmap) {
        kprintf("PMM_DEBUG: bitmap is NULL\n");
        return;
    }
    uint64_t start_page = addr / PAGE_SIZE;
    uint64_t end_page = (addr + bytes - 1) / PAGE_SIZE;
    uint64_t bitmap_size = (total_pages + 7) / 8;

        kprintf("PMM_DEBUG: addr=%p bytes=%d start_page=0x%x end_page=0x%x total_pages=0x%x bitmap=%p bitmap_size=%d\n",
            (void*)addr, (int)bytes, (int)start_page, (int)end_page, (int)total_pages, bitmap, (int)bitmap_size);

    if (end_page >= total_pages) kprintf("PMM_DEBUG: RANGE OUT OF RAM (end_page >= total_pages)\n");

    uint64_t bstart = (start_page / 8 >= 2) ? (start_page / 8 - 2) : 0;
    uint64_t bend = bstart + 8;
    kprintf("PMM_DEBUG bitmap[%d..%d]:", (int)bstart, (int)bend);
    for (uint64_t i = bstart; i <= bend && i < bitmap_size; i++) {
        kprintf(" %x", (int)bitmap[i]);
    }
    kprintf("\n");

    // Dump first 32 bytes at addr (safe-ish; caller should ensure it's valid)
    uint8_t* p = (uint8_t*)addr;
    kprintf("PMM_DEBUG bytes at addr %p:", (void*)addr);
    for (int i = 0; i < 32; i++) {
        kprintf(" %x", (int)p[i]);
    }
    kprintf("\n");
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

    // 3. Place bitmap at high memory (end of RAM) to avoid overlapping
    //    dynamic allocations placed after kernel. Compute bitmap pages
    //    and place the bitmap at the top of usable RAM.
    uint64_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (bitmap_pages + 1 >= total_pages) {
        // Too little memory to place bitmap — fallback to after kernel
        bitmap = (uint8_t*)kernel_end_ptr;
    } else {
        uint64_t bitmap_page_start = total_pages - bitmap_pages;
        bitmap = (uint8_t*)(bitmap_page_start * PAGE_SIZE);
    }

    kprintf("PMM INIT: mem_bytes=%d PAGE_SIZE=%d total_pages=%d bitmap=%p bitmap_size=%d kernel_end=%p\n",
            (int)mem_size_in_bytes, (int)PAGE_SIZE, (int)total_pages, bitmap, (int)bitmap_size, (void*)kernel_end_ptr);

    // Compute reserved low pages used by kernel
    uint64_t kernel_reserved_pages = (kernel_end_ptr + PAGE_SIZE - 1) / PAGE_SIZE;
    // Add a guard of extra reserved pages so kmalloc/pmm allocations won't land
    // immediately after the kernel (helps avoid accidental overlap)
    pmm_reserved_pages = kernel_reserved_pages + PMM_RESERVED_GUARD_PAGES;

    // If bitmap is at high memory, also reserve its pages
    uint64_t bitmap_page_start = ((uint64_t)bitmap) / PAGE_SIZE;
    uint64_t reserved_end = pmm_reserved_pages * PAGE_SIZE;
    kprintf("PMM INIT: bitmap_page_start=%d bitmap_pages=%d pmm_reserved_pages(low)=%d reserved_end(low)=%p (guard=%u pages)\n",
            (int)bitmap_page_start, (int)bitmap_pages, (int)(pmm_reserved_pages), (void*)reserved_end, PMM_RESERVED_GUARD_PAGES);

    // 4. Initialize bitmap
    for(uint64_t i = 0; i < bitmap_size; i++) bitmap[i] = 0;

    // 5. Reserve Kernel + Bitmap area
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