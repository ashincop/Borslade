#include "keyboard.h"
#include <drivers/screen/fb.h>
volatile char last_char = 0;

static char internal_buffer[256];
static int buffer_idx = 0;

void kscan(char* buf) {
    buffer_idx = 0; // Reset internal buffer for a new line
    
    while (1) {
        // Wait for a character to appear from the interrupt
        if (last_char != 0) {
            char c = last_char;
            last_char = 0; // "Acknowledge" and consume the char

            if (c == '\n') {
                internal_buffer[buffer_idx] = '\0';
                
                // Copy internal result to the user's provided buffer
                int i = 0;
                while (internal_buffer[i] != '\0') {
                    buf[i] = internal_buffer[i];
                    i++;
                }
                buf[i] = '\0'; // Null terminate the user's buffer
                
                kprintf("\n");
                return; // Exit function, line is complete
            } 
            else if (c == '\b' && buffer_idx > 0) {
                buffer_idx--;
                // If you have a backspace visual function: fb_backspace();
            } 
            else if (buffer_idx < 255) {
                internal_buffer[buffer_idx++] = c;
                kprintf("%c", c); // Echo to screen so user sees what they type
            }
        }
        
        // Efficiency: Tell the CPU to pause until the next interrupt
        asm volatile("hlt"); 
    }
}
// A very basic US-QWERTY mapping (Scan Code Set 1)
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' ', 0
};
static inline void outb(uint16_t port, uint8_t val) {
    // "a" (val) puts the value in the AL register
    // "Nd" (port) puts the port in the DX register
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    // "out" (=a) tells the compiler to take the result from AL
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}
void keyboard_init() {
    // 1. Wait for the controller to be ready
    while(inb(0x64) & 2); 
    
    // 2. Send "Enable Scanning" command to the keyboard
    outb(0x60, 0xF4); 
    
    // Note: The PIC unmasking should already be done in your pic_remap!
}
void keyboard_handler(uint64_t *stack_anchor) {
    // Read the scancode from the keyboard's data port
    uint8_t scancode = inb(0x60);

    // Bit 7 (0x80) is set if the key was RELEASED.
    // We usually only care about when a key is PRESSED (Bit 7 is clear).
    if (!(scancode & 0x80)) {
        if (scancode < 128 && kbd_us[scancode] != 0) {
            char c = kbd_us[scancode];
            last_char = c;
        }
    }
    
    // The EOI is handled by your common EOI logic we built earlier
}