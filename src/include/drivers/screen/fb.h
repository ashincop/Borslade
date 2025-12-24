#pragma once
#include <stdint.h>
#include <arch/x86_64/types.h>
void init_gop(uint64_t mbi_addr);
void kprintf(const char* fmt, ...);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void clear_screen(uint32_t color);