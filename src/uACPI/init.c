#include <stdint.h>
#include <stdint.h>
#include <stddef.h>

// Multiboot2 Tag Types for ACPI
#define MB2_TAG_TYPE_END       0
#define MB2_TAG_TYPE_ACPI_OLD  14
#define MB2_TAG_TYPE_ACPI_NEW  15
#define MB2_TAG_ALIGN          8

struct mb2_tag {
    uint32_t type;
    uint32_t size;
};

struct mb2_info_header {
    uint32_t total_size;
    uint32_t reserved;
};

// This matches the uACPI expectation for uacpi_kernel_get_rsdp
void* find_xsdp_from_mb2(uint64_t mbi_phys_addr) {
    if (mbi_phys_addr == 0) return NULL;

    struct mb2_info_header *mbi = (struct mb2_info_header*)(uintptr_t)mbi_phys_addr;
    struct mb2_tag *tag = (struct mb2_tag*)(uintptr_t)(mbi_phys_addr + 8);

    void* rsdp_v1 = NULL;

    // Iterate through tags until we hit the END tag (Type 0)
    while (tag->type != MB2_TAG_TYPE_END) {
        
        // Priority 1: New ACPI (v2.0+) contains the XSDP
        if (tag->type == MB2_TAG_TYPE_ACPI_NEW) {
            // The RSDP structure starts right after the 8-byte tag header
            return (void*)((uintptr_t)tag + 8);
        }

        // Priority 2: Old ACPI (v1.0)
        if (tag->type == MB2_TAG_TYPE_ACPI_OLD) {
            rsdp_v1 = (void*)((uintptr_t)tag + 8);
        }

        // Move to the next tag (tags are 8-byte aligned)
        uintptr_t next_addr = (uintptr_t)tag + tag->size;
        tag = (struct mb2_tag*)((next_addr + (MB2_TAG_ALIGN - 1)) & ~(MB2_TAG_ALIGN - 1));
        
        // Safety check to prevent reading past the MBI total size
        if ((uintptr_t)tag >= (mbi_phys_addr + mbi->total_size)) break;
    }

    // Fallback to v1.0 if v2.0 wasn't found
    return rsdp_v1;
}