#pragma once
#include <stddef.h>
#include <stdint.h>

// PS/2 Keyboard commands/responses
#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_SELFTEST 0xFF
#define PS2_ECHO 0xEE

// PS/2 Controller ports
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

// Status flags
#define PS2_OBF 0x01 // Output buffer full
#define PS2_IBF 0x02 // Input buffer full
#define PS2_AUX 0x20 // Mouse data
#define PS2_TIMEOUT 0x40
#define PS2_PARITYERR 0x80

// Keyboard driver state
typedef struct {
	uint8_t buffer[16]; // command queue
	size_t buf_head;
	size_t buf_tail;
	uint8_t shift;
	uint8_t ctrl;
	uint8_t alt;
	uint8_t capslock;
	uint8_t numlock;
	uint8_t scrolllock;
	uint8_t extended; // 0xE0 prefix state
	uint8_t released; // 0xF0 break state
} ps2kbd_t;

// Ring buffer for key presses
#define PS2KBD_BUFFER_SIZE 256
typedef struct {
	uint8_t buf[PS2KBD_BUFFER_SIZE];
	size_t head, tail;
} ps2kbd_buffer_t;

// API
void ps2kbd_init(void);
void ps2kbd_irq(void);
uint8_t ps2kbd_get(void);
void ps2kbd_push(uint8_t key);
size_t ps2kbd_kscan_line(char *buf, size_t maxlen);
uint8_t ps2kbd_kscan(void);
extern const uint8_t sc2key[256];
extern const uint8_t sc2key_ext[256];