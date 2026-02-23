#pragma once
#include <stdarg.h>
#include <stdint.h>
struct multiboot_tag {
	uint32_t type;
	uint32_t size;
};

struct multiboot_tag_framebuffer {
	uint32_t type;
	uint32_t size;
	uint64_t addr; // <-- THIS IS YOUR FRAMEBUFFER START
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
	uint8_t type_fb;
	uint16_t reserved;
};
void init_gop(uint64_t mbi_addr);
void kprintf(const char *fmt, ...);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void clear_screen(uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
void vkprintf(const char *fmt, va_list args);
int init_serial();
void kprint_s(const char *s);