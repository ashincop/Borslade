#include <stdint.h>
#include <stdbool.h>
#include <drivers/screen/fb.h>
#include <utils/misc/inline.h>

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_CMD 0x64

#define SCREEN_W 1280   // change to your real res
#define SCREEN_H 720

#define CURSOR_SIZE 8
#define CURSOR_COLOR 0xFFFFFF
#define BG_COLOR 0x000000
static int mouse_x = SCREEN_W / 2;
static int mouse_y = SCREEN_H / 2;
static int prev_mouse_x = SCREEN_W / 2;
static int prev_mouse_y = SCREEN_H / 2;
extern void draw_rect(int x, int y, int width, int height, uint32_t color);

// ---------------- Mouse State ----------------
struct MouseState {
    int8_t dx;
    int8_t dy;
    uint8_t buttons; // bit 0 = left, bit 1 = right, bit 2 = middle
};

static struct MouseState mouse;

// PS/2 mouse packet state
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

// ---------------- Read Mouse ----------------
void mouse_wait_input() {
    while (!(inb(PS2_STATUS) & 0x01));
}

void mouse_wait_output() {
    while (inb(PS2_STATUS) & 0x02);
}

// Send command to mouse
void mouse_write(uint8_t cmd) {
    mouse_wait_output();
    outb(0x64, 0xD4); // tell PS/2 controller we send to mouse
    mouse_wait_output();
    outb(0x60, cmd);
}

// ---------------- IRQ Handler ----------------
void mouse_irq_handler() {
    uint8_t data = inb(PS2_DATA);
    kprintf("THE DATA: %d\n", data);
    switch (mouse_cycle) {
        case 0:
            mouse_packet[0] = data;
            mouse_cycle++;
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_packet[2] = data;
            mouse_cycle = 0;

            // parse packet
            mouse.buttons = mouse_packet[0] & 0x07;
            mouse.dx = mouse_packet[1];
            mouse.dy = mouse_packet[2];

            // invert Y if needed (typical PS/2 is negative up)
            mouse.dy = -mouse.dy;
            // erase old cursor
            draw_rect(prev_mouse_x, prev_mouse_y, CURSOR_SIZE, CURSOR_SIZE, BG_COLOR);

            // update position
            prev_mouse_x = mouse_x;
            prev_mouse_y = mouse_y;

            mouse_x += mouse.dx;
            mouse_y += mouse.dy;

            // clamp to screen
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x > SCREEN_W - CURSOR_SIZE) mouse_x = SCREEN_W - CURSOR_SIZE;
            if (mouse_y > SCREEN_H - CURSOR_SIZE) mouse_y = SCREEN_H - CURSOR_SIZE;

            // draw new cursor
            draw_rect(mouse_x, mouse_y, CURSOR_SIZE, CURSOR_SIZE, CURSOR_COLOR);


            // optional: call a callback here
            // on_mouse_event(mouse.dx, mouse.dy, mouse.buttons);
            break;
    }

    // send EOI to PIC
    outb(0x20, 0x20);
}

// ---------------- Init ----------------
void mouse_init() {
    // enable auxiliary device
    mouse_wait_output();
    outb(0x64, 0xA8); // enable mouse
    // reset mouse
    mouse_write(0xF6); // set default
    mouse_write(0xF4); // enable streaming
}
