#pragma once
#include <stdint.h>
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
typedef struct {
	uint8_t e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry; // THIS IS WHERE WE JUMP
	uint64_t e_phoff; // Program Header Offset
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum; // Number of Program Headers
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr; // WHERE TO PUT IT IN MEMORY
	uint64_t p_paddr;
	uint64_t p_filesz; // HOW MUCH DATA IS IN THE FILE
	uint64_t p_memsz;  // HOW MUCH SPACE IT NEEDS (for .bss)
	uint64_t p_align;
} Elf64_Phdr;

#define PT_LOAD 1
uint64_t load_elf_pie(void *elf_data, uint64_t load_base);
// Segment types
#define PT_LOAD 1
#define PT_DYNAMIC 2

// Dynamic table tags
#define DT_NULL 0
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9

// Relocation macro
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)

// Dynamic section entry
typedef struct {
	int64_t d_tag;
	union {
		uint64_t d_val;
		uint64_t d_ptr;
	} d_un;
} Elf64_Dyn;

// Relocation entry with addend
typedef struct {
	uint64_t r_offset;
	uint64_t r_info;
	int64_t r_addend;
} Elf64_Rela;