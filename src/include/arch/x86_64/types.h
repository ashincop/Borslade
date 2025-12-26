#pragma once
#include "stdint.h"
struct gdtr {
    uint16_t size; // size of GDT (sizeof) - 1
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
struct idtr {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));
struct InterruptDescriptor64 {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
   uint32_t offset_3;        // offset bits 32..63
   uint32_t zero;            // reserved
} __attribute__((packed));
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;      // <-- THIS IS YOUR FRAMEBUFFER START
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type_fb;
    uint16_t reserved;
};
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;      // <--- THE MOST IMPORTANT: Kernel Stack for Ring 0
    uint64_t rsp1;      // Stack for Ring 1 (unused)
    uint64_t rsp2;      // Stack for Ring 2 (unused)
    uint64_t reserved1;
    uint64_t ist[7];    // Interrupt Stack Table (for special cases like Double Faults)
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
typedef struct task {
    void* stack_ptr;        // Offset 0: Saved RSP
    uint64_t cr3;           // Offset 8: Page Table base
    struct task* next;      // Offset 16: Next in list
    int id;                 // Offset 24: Debug ID
    uint64_t kernel_stack_top; // Offset 32: Landing pad for Ring 3 -> Ring 0
    char* name;
    
} task_t __attribute__((packed));
typedef struct malloc_header {
    uint32_t magic;
    uint64_t size;
    int is_free;
    struct malloc_header* next;
} malloc_header_t;
typedef enum { VFS_FILE, VFS_DIRECTORY, VFS_DEVICE, VFS_PIPE } vnode_type_t;

struct vnode {
    vnode_type_t type;
    uint32_t size;
    void* private_data; // Points to the actual FS (e.g., TarFS or Ext2)
    struct vfs_entry* ops; // Function pointers for read/write
};

struct vfs_entry {
    int (*read)(struct vnode* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    int (*write)(struct vnode* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    // Add open, close, readdir later
};
struct vfs_node {
    char name[128];
    struct vnode* vnode;
    struct vfs_node* parent;
    struct vfs_node* children; // Pointer to first child
    struct vfs_node* next;     // Pointer to sibling
};

struct mb2_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string[]; // Name of the module (e.g., "initrd.img")
};