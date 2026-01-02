#include <drivers/keyboard/keyboard.h>
#include <drivers/screen/fb.h>
#include <stdint.h>
#include <utils/misc/inline.h>
#include <utils/logging/log.h>
#define host "com.strawberry.drivers.keyboard"

// Volatile is correct here as this changes inside an ISR
volatile char last_char = 0;

static char internal_buffer[256];
static int buffer_idx = 0;

/* --- Keyboard Logic --- */

// US-QWERTY Mapping
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ', 0
};

void keyboard_init() {
    log(host, O_OKAY, "Initialized ps2k!\n");
    // 1. Disable both ports so they don't scream while we configure
    while (inb(0x64) & 2); outb(0x64, 0xAD); // Disable Kbd
    while (inb(0x64) & 2); outb(0x64, 0xA7); // Disable Mouse

    // 2. Flush the buffer (Drain the "triggered" data)
    while (inb(0x64) & 1) { inb(0x60); }

    // 3. Read the Controller Configuration Byte
    while (inb(0x64) & 2); outb(0x64, 0x20);
    while (!(inb(0x64) & 1));
    uint8_t config = inb(0x60);

    // 4. Enable Interrupts for both (Bit 0 = Kbd, Bit 1 = Mouse)
    config |= (1 << 0) | (1 << 1); 
    config &= ~(1 << 4); // Ensure Kbd clock is NOT disabled
    config &= ~(1 << 5); // Ensure Mouse clock is NOT disabled

    // 5. Write the Config back
    while (inb(0x64) & 2); outb(0x64, 0x60);
    while (inb(0x64) & 2); outb(0x60, config);

    // 6. Re-enable the ports
    while (inb(0x64) & 2); outb(0x64, 0xAE); // Enable Kbd
    while (inb(0x64) & 2); outb(0x64, 0xA8); // Enable Mouse

    // 7. Tell the Keyboard device to start scanning
    while (inb(0x64) & 2); outb(0x60, 0xF4);
}

void keyboard_handler(uint64_t *stack_anchor) {
    uint8_t scancode = inb(0x60);
    // If the top bit is clear, it's a "Make" (press) event
    if (!(scancode & 0x80)) {
        if (scancode < 128 && kbd_us[scancode] != 0) {
            last_char = kbd_us[scancode];
        }
    }
}

/* --- Shell Input Function --- */

void kscan(char* buf) {
    buffer_idx = 0;
    
    while (1) {
        if (last_char != 0) {
            char c = last_char;
            last_char = 0; 

            if (c == '\n') {
                internal_buffer[buffer_idx] = '\0';
                
                // Transfer to user buffer
                for (int i = 0; i <= buffer_idx; i++) {
                    buf[i] = internal_buffer[i];
                }
                
                kprintf("\n");
                return;
            } 
            else if (c == '\b') {
                if (buffer_idx > 0) {
                    buffer_idx--;
                    kprintf("\b \b"); // Move back, overwrite with space, move back again
                }
            } 
            else if (buffer_idx < 254) { // Save room for null terminator
                internal_buffer[buffer_idx++] = c;
                kprintf("%c", c);
            }
        }
        
    }
}