#include <drivers/storage/vfs/vfs.h>
#include <drivers/screen/fb.h>
#include <arch/x86_64/alloc.h>
#include <stddef.h>
#include <stdint.h>

/* --- Constants & Types --- */
#define MAX_OPEN_FILES 32
#define VFS_CHUNK_SIZE 128
#define VFS_MAX_LIMIT (4 * 1024 * 1024) // 4MB
#define MAX_VFS_MEMORY (4 * 1024 * 1024) // 4 Megabytes
typedef struct {
    int vfs_index;    
    uint32_t offset;  
    uint8_t used;     
} fd_t;

typedef struct {
    char name[64];
    void* data;
    uint32_t size;
    uint8_t used;
} vfs_node_t;

struct cpio_header {
    char magic[6];
    char ino[8], mode[8], uid[8], gid[8], nlink[8], mtime[8], filesize[8];
    char devmajor[8], devminor[8], rdevmajor[8], rdevminor[8], namesize[8], check[8];
} __attribute__((packed));

/* --- Global State --- */
uint64_t vfs_start = 0;
uint64_t vfs_end = 0;
uint64_t vfs_size = 0;

vfs_node_t* file_table = NULL;
int table_capacity = 0;
int total_files = 0;
fd_t fd_table[MAX_OPEN_FILES];

/* --- String/Mem Helpers --- */
extern void* memset(void* s, int c, size_t n);

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) if (p1[i] != p2[i]) return p1[i] - p2[i];
    return 0;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/* --- Hex Conversion --- */
static uint32_t hex_to_u32(char *s, int len) {
    uint32_t res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 4;
        if (s[i] >= '0' && s[i] <= '9') res += s[i] - '0';
        else if (s[i] >= 'A' && s[i] <= 'F') res += s[i] - 'A' + 10;
        else if (s[i] >= 'a' && s[i] <= 'f') res += s[i] - 'a' + 10;
    }
    return res;
}

static void u32_to_hex8(uint32_t val, char* dest) {
    const char* hex_chars = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        dest[i] = hex_chars[val & 0xF];
        val >>= 4;
    }
}

/* --- Dynamic Table Logic --- */
int grow_file_table() {
    int new_capacity = table_capacity + 128;
    uint64_t new_size = (uint64_t)new_capacity * sizeof(vfs_node_t);

    if (new_size > MAX_VFS_MEMORY) return 0;

    vfs_node_t* new_table = (vfs_node_t*)kmalloc(new_size);
    if (!new_table) return 0;

    memset(new_table, 0, new_size);

    if (file_table) {
        // Copy old entries to the new table
        memcpy(new_table, file_table, table_capacity * sizeof(vfs_node_t));
        // kfree(file_table); // REMOVED: Preventing heap corruption
    }

    file_table = new_table;
    table_capacity = new_capacity;
    return 1;
}

/* --- The Indexer --- */
void vfs_mount() {
    uintptr_t curr = (uintptr_t)vfs_start;
    total_files = 0;

    if (!file_table) grow_file_table();

    while (curr + 110 <= vfs_end) {
        if (total_files >= table_capacity) {
            if (!grow_file_table()) break;
        }

        struct cpio_header* h = (struct cpio_header*)curr;
        if (memcmp(h->magic, "070701", 6) != 0) break;

        uint32_t n_size = hex_to_u32(h->namesize, 8);
        uint32_t f_size = hex_to_u32(h->filesize, 8);
        char* name_ptr = (char*)(curr + 110);

        if (memcmp(name_ptr, "TRAILER!!!", 10) == 0) break;

        memset(file_table[total_files].name, 0, 64);
        memcpy(file_table[total_files].name, name_ptr, (n_size > 63) ? 63 : n_size);
        
        uintptr_t data_addr = (curr + 110 + n_size + 3) & ~3;
        file_table[total_files].data = (void*)data_addr;
        file_table[total_files].size = f_size;
        file_table[total_files].used = 1;
        
        total_files++;
        curr = (data_addr + f_size + 3) & ~3;
    }
    kprintf("VFS: Mounted %d files. Cap: %d\n", total_files, table_capacity);
}

/* --- VFS API --- */
int path_match(const char* table_name, const char* search_name) {
    if (search_name[0] == '.' && search_name[1] == '/') search_name += 2;
    else if (search_name[0] == '/') search_name++;

    if (table_name[0] == '.' && table_name[1] == '/') table_name += 2;
    else if (table_name[0] == '/') table_name++;

    while (*table_name && *search_name) {
        if (*table_name != *search_name) break;
        table_name++;
        search_name++;
    }
    return (*table_name == *search_name);
}

void init_vfs(uint64_t addr) {
    kprintf("MBI Pointer: %p\n", addr);
    
    struct multiboot_tag* tag = (struct multiboot_tag*)(addr + 8);
    while (tag->type != 0) {
        if (tag->type == 3) {
            struct mb2_tag_module* mod = (struct mb2_tag_module*)tag;
            
            // IF THESE ARE > 4GB (0x100000000), YOUR PAGING WILL CRASH
            kprintf("Module found: %p - %p\n", mod->mod_start, mod->mod_end);
            
            if (mod->mod_start >= 0x100000000) {
                kprintf("CRITICAL ERROR: Module loaded above 4GB limit!\n");
                for(;;);
            }
            vfs_start = mod->mod_start;
            vfs_end = mod->mod_end;
            vfs_size = vfs_end - vfs_start;
            vfs_mount();
        }
        tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
}

int vfs_read(const char* filename, char* out_buffer, uint32_t max_len) {
    for (int i = 0; i < total_files; i++) {
        if (file_table[i].used && path_match(file_table[i].name, filename)) {
            uint32_t to_copy = (file_table[i].size >= max_len) ? max_len - 1 : file_table[i].size;
            memcpy(out_buffer, file_table[i].data, to_copy);
            out_buffer[to_copy] = '\0';
            return to_copy;
        }
    }
    return -1;
}

uint32_t vfs_get_filesize(const char* filename) {
    kprintf("Get file size of: %s\n", filename);
    for (int i = 0; i < total_files; i++) {
        if (file_table[i].used && path_match(file_table[i].name, filename)) return file_table[i].size;
    }
    return 0; 
}

int sys_open(const char* filename) {
    for (int i = 0; i < total_files; i++) {
        if (file_table[i].used && path_match(file_table[i].name, filename)) {
            for (int j = 0; j < MAX_OPEN_FILES; j++) {
                if (!fd_table[j].used) {
                    fd_table[j].vfs_index = i;
                    fd_table[j].offset = 0;
                    fd_table[j].used = 1;
                    return j;
                }
            }
        }
    }
    return -1;
}

int sys_read(int fd, char* buf, uint32_t len) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) return -1;
    vfs_node_t* node = &file_table[fd_table[fd].vfs_index];
    uint32_t remaining = node->size - fd_table[fd].offset;
    uint32_t to_read = (len > remaining) ? remaining : len;
    memcpy(buf, (uint8_t*)node->data + fd_table[fd].offset, to_read);
    fd_table[fd].offset += to_read;
    return to_read;
}

int sys_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    fd_table[fd].used = 0;
    return 0;
}
int sys_lseek(int fd, int offset, int whence) {
    // 1. Validate the File Descriptor
    if (fd < 0 || fd >= MAX_OPEN_FILES || !fd_table[fd].used) {
        return -1;
    }
    
    vfs_node_t* node = &file_table[fd_table[fd].vfs_index];
    uint32_t new_offset = fd_table[fd].offset;

    // 2. Calculate the new offset based on 'whence'
    if (whence == 0) {         // SEEK_SET: relative to start
        new_offset = (uint32_t)offset;
    } 
    else if (whence == 1) {    // SEEK_CUR: relative to current position
        new_offset += offset;
    } 
    else if (whence == 2) {    // SEEK_END: relative to end of file
        new_offset = node->size + offset;
    } 
    else {
        return -1; // Invalid whence
    }

    // 3. Safety Check: Don't allow seeking past the end or before the start
    if (new_offset > node->size) new_offset = node->size;
    // (Note: Since new_offset is uint32_t, it can't be negative; 
    // it would wrap to a huge number and hit the 'node->size' check above)

    fd_table[fd].offset = new_offset;
    return (int)new_offset;
}