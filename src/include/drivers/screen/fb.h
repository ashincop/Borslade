#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <arch/x86_64/types.h>
void init_gop(uint64_t mbi_addr);
void kprintf(const char* fmt, ...);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void clear_screen(uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
void vkprintf(const char* fmt, va_list args);
int init_serial();
void kprint_s(const char* s);