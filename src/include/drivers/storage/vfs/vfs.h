#pragma once
#include <stdint.h>
#include <arch/x86_64/types.h>
void init_vfs(uint64_t addr);
int vfs_read(const char* filename, char* out_buffer, uint32_t max_len);
int vfs_write(const char* filename, void* new_data, uint32_t new_size);
int sys_open(const char* filename);
int sys_read(int fd, void* buf, uint32_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
uint32_t vfs_get_filesize(const char* filename);