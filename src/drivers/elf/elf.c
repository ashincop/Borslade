#include <utils/logging/log.h>
#include <drivers/elf/elf.h>
#include <arch/x86_64/alloc.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <utils/mem/mem.h>
#include <drivers/screen/fb.h>

// Page size constants
#define PAGE_SIZE 4096
#define PAGE_MASK 0xFFF
#define PFN_SHIFT 12
#define host "com.strawberry.exec.elf64"
// ELF constants
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define EM_X86_64 0x3E

uint64_t load_elf_pie(void* elf_data, uint64_t preferred_base) {
    Elf64_Ehdr* header = (Elf64_Ehdr*)elf_data;
    
    // Validate ELF
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3 ||
        header->e_machine != EM_X86_64) {
        kprintf("[%U ERROR %Q] Invalid ELF64 binary\n");
        return 0;
    }
    
    uintptr_t phdr_table_addr = (uintptr_t)elf_data + header->e_phoff;
    
    // Find ELF base (lowest PT_LOAD p_vaddr)
    uint64_t elf_base = UINT64_MAX;
    for (int i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)(phdr_table_addr + (i * header->e_phentsize));
        if (phdr->p_type == PT_LOAD && phdr->p_vaddr < elf_base) {
            elf_base = phdr->p_vaddr;
        }
    }
    
    if (elf_base == UINT64_MAX) {
        kprintf("[%U ERROR %Q] No PT_LOAD segments found\n");
        return 0;
    }
    
    log(host, O_OKAY, "ELF: %d program headers, base=0x%p\n", header->e_phnum, elf_base);
    
    // Load segments
    for (int i = 0; i < header->e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)(phdr_table_addr + (i * header->e_phentsize));
        
        if (phdr->p_type == PT_LOAD) {
            // Calculate exact virtual addresses (relocated)
            uint64_t vaddr_start = preferred_base + (phdr->p_vaddr - elf_base);
            uint64_t vaddr_end = vaddr_start + phdr->p_memsz;
            
            // Align to page boundaries for PMM
            uint64_t page_start = vaddr_start & ~((uint64_t)PAGE_MASK);
            uint64_t page_end = (vaddr_end + PAGE_MASK) & ~((uint64_t)PAGE_MASK);
            
            log(host, O_OKAY, "PT_LOAD[%d]: VA=0x%p-0x%p (pages=0x%p-0x%p)\n", i, vaddr_start, vaddr_end, page_start, page_end);
            
            // Clear memory (p_memsz covers BSS/padding)
            memset((void*)vaddr_start, 0, phdr->p_memsz);
            
            // Copy file data (p_filesz <= p_memsz)
            if (phdr->p_filesz > 0) {
                memcpy((void*)vaddr_start, 
                       (void*)((uintptr_t)elf_data + phdr->p_offset), 
                       phdr->p_filesz);
            }
            
            log(host, O_OKAY, "Loaded segment %d\n", i);
        }
    }
    
    // Calculate correct entry point (relocated)
    uint64_t entry_offset = header->e_entry - elf_base;
    uint64_t entry = preferred_base + entry_offset;
    
    log(host, O_OKAY, "ELF base=0x%p, e_entry=0x%p, offset=0x%p, final entry=0x%p\n", elf_base, header->e_entry, entry_offset, entry);
    
    return entry;
}
