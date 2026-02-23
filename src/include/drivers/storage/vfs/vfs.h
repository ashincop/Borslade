#pragma once
#include <stdint.h>
typedef enum { VFS_FILE, VFS_DIRECTORY, VFS_DEVICE, VFS_PIPE } vnode_type_t;

struct vnode {
	vnode_type_t type;
	uint32_t size;
	void *private_data;    // Points to the actual FS (e.g., TarFS or Ext2)
	struct vfs_entry *ops; // Function pointers for read/write
};

struct vfs_entry {
	int (*read)(struct vnode *node, uint32_t offset, uint32_t size,
		    uint8_t *buffer);
	int (*write)(struct vnode *node, uint32_t offset, uint32_t size,
		     uint8_t *buffer);
	// Add open, close, readdir later
};
struct vfs_node {
	char name[128];
	struct vnode *vnode;
	struct vfs_node *parent;
	struct vfs_node *children; // Pointer to first child
	struct vfs_node *next;	   // Pointer to sibling
};

struct mb2_tag_module {
	uint32_t type;
	uint32_t size;
	uint32_t mod_start;
	uint32_t mod_end;
	char string[]; // Name of the module (e.g., "initrd.img")
};
void init_vfs(uint64_t addr);
int vfs_read(const char *filename, char *out_buffer, uint32_t max_len);
int vfs_write(const char *filename, void *new_data, uint32_t new_size);
int sys_open(const char *filename);
int sys_read(int fd, void *buf, uint32_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
uint32_t vfs_get_filesize(const char *filename);
int vfs_list_dir(const char *path, char out[][64], int max);