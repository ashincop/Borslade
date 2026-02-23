#include <drivers/keyboard/keyboard.h>
#include <utils/misc/inline.h> // inb/outb helpers

// --- Keycodes ---
enum {
	KEY_NONE = 0x00,
	KEY_ESC,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_Q,
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_T,
	KEY_Y,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_ENTER,
	KEY_LCTRL,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_GRAVE,
	KEY_LSHIFT,
	KEY_BACKSLASH,
	KEY_Z,
	KEY_X,
	KEY_C,
	KEY_V,
	KEY_B,
	KEY_N,
	KEY_M,
	KEY_COMMA,
	KEY_DOT,
	KEY_SLASH,
	KEY_RSHIFT,
	KEY_KP_ASTERISK,
	KEY_LALT,
	KEY_SPACE,
	KEY_CAPSLOCK,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_NUMLOCK,
	KEY_SCROLLLOCK,
	KEY_KP_7,
	KEY_KP_8,
	KEY_KP_9,
	KEY_KP_MINUS,
	KEY_KP_4,
	KEY_KP_5,
	KEY_KP_6,
	KEY_KP_PLUS,
	KEY_KP_1,
	KEY_KP_2,
	KEY_KP_3,
	KEY_KP_0,
	KEY_KP_DOT,
	KEY_F11,
	KEY_F12,
	KEY_RCTRL,
	KEY_RALT,
	KEY_HOME,
	KEY_UP,
	KEY_PAGEUP,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_END,
	KEY_DOWN,
	KEY_PAGEDOWN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_LGUI,
	KEY_RGUI,
	KEY_APPS,
	KEY_PAUSE,
	KEY_PRINTSCREEN
};

// --- Scan code tables ---
const uint8_t sc2key[256] = {
    [0x01] = KEY_ESC,	      [0x02] = KEY_1,	      [0x03] = KEY_2,
    [0x04] = KEY_3,	      [0x05] = KEY_4,	      [0x06] = KEY_5,
    [0x07] = KEY_6,	      [0x08] = KEY_7,	      [0x09] = KEY_8,
    [0x0A] = KEY_9,	      [0x0B] = KEY_0,	      [0x0C] = KEY_MINUS,
    [0x0D] = KEY_EQUAL,	      [0x0E] = KEY_BACKSPACE, [0x0F] = KEY_TAB,
    [0x10] = KEY_Q,	      [0x11] = KEY_W,	      [0x12] = KEY_E,
    [0x13] = KEY_R,	      [0x14] = KEY_T,	      [0x15] = KEY_Y,
    [0x16] = KEY_U,	      [0x17] = KEY_I,	      [0x18] = KEY_O,
    [0x19] = KEY_P,	      [0x1A] = KEY_LBRACKET,  [0x1B] = KEY_RBRACKET,
    [0x1C] = KEY_ENTER,	      [0x1D] = KEY_LCTRL,     [0x1E] = KEY_A,
    [0x1F] = KEY_S,	      [0x20] = KEY_D,	      [0x21] = KEY_F,
    [0x22] = KEY_G,	      [0x23] = KEY_H,	      [0x24] = KEY_J,
    [0x25] = KEY_K,	      [0x26] = KEY_L,	      [0x27] = KEY_SEMICOLON,
    [0x28] = KEY_APOSTROPHE,  [0x29] = KEY_GRAVE,     [0x2A] = KEY_LSHIFT,
    [0x2B] = KEY_BACKSLASH,   [0x2C] = KEY_Z,	      [0x2D] = KEY_X,
    [0x2E] = KEY_C,	      [0x2F] = KEY_V,	      [0x30] = KEY_B,
    [0x31] = KEY_N,	      [0x32] = KEY_M,	      [0x33] = KEY_COMMA,
    [0x34] = KEY_DOT,	      [0x35] = KEY_SLASH,     [0x36] = KEY_RSHIFT,
    [0x37] = KEY_KP_ASTERISK, [0x38] = KEY_LALT,      [0x39] = KEY_SPACE,
    [0x3A] = KEY_CAPSLOCK,    [0x3B] = KEY_F1,	      [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,	      [0x3E] = KEY_F4,	      [0x3F] = KEY_F5,
    [0x40] = KEY_F6,	      [0x41] = KEY_F7,	      [0x42] = KEY_F8,
    [0x43] = KEY_F9,	      [0x44] = KEY_F10,	      [0x45] = KEY_NUMLOCK,
    [0x46] = KEY_SCROLLLOCK,  [0x47] = KEY_KP_7,      [0x48] = KEY_KP_8,
    [0x49] = KEY_KP_9,	      [0x4A] = KEY_KP_MINUS,  [0x4B] = KEY_KP_4,
    [0x4C] = KEY_KP_5,	      [0x4D] = KEY_KP_6,      [0x4E] = KEY_KP_PLUS,
    [0x4F] = KEY_KP_1,	      [0x50] = KEY_KP_2,      [0x51] = KEY_KP_3,
    [0x52] = KEY_KP_0,	      [0x53] = KEY_KP_DOT,    [0x57] = KEY_F11,
    [0x58] = KEY_F12};

const uint8_t sc2key_ext[256] = {
    [0x1D] = KEY_RCTRL,	      [0x38] = KEY_RALT,   [0x47] = KEY_HOME,
    [0x48] = KEY_UP,	      [0x49] = KEY_PAGEUP, [0x4B] = KEY_LEFT,
    [0x4D] = KEY_RIGHT,	      [0x4F] = KEY_END,	   [0x50] = KEY_DOWN,
    [0x51] = KEY_PAGEDOWN,    [0x52] = KEY_INSERT, [0x53] = KEY_DELETE,
    [0x5B] = KEY_LGUI,	      [0x5C] = KEY_RGUI,   [0x5D] = KEY_APPS,
    [0x37] = KEY_PRINTSCREEN, [0xE1] = KEY_PAUSE};

// --- Driver ring buffer ---
static ps2kbd_buffer_t kbd_buf = {0};

void ps2kbd_push(uint8_t key)
{
	size_t next = (kbd_buf.head + 1) % PS2KBD_BUFFER_SIZE;
	if (next != kbd_buf.tail) {
		kbd_buf.buf[kbd_buf.head] = key;
		kbd_buf.head = next;
	}
}

uint8_t ps2kbd_get(void)
{
	if (kbd_buf.head == kbd_buf.tail)
		return 0;
	uint8_t key = kbd_buf.buf[kbd_buf.tail];
	kbd_buf.tail = (kbd_buf.tail + 1) % PS2KBD_BUFFER_SIZE;
	return key;
}

// --- non-blocking: returns 0 if no key ---
uint8_t ps2kbd_kscan_nonblocking(void)
{
	return ps2kbd_get(); // just grab next key, 0 if empty
}

// --- blocking: waits until a key is pressed ---
uint8_t ps2kbd_kscan(void)
{
	uint8_t key;
	while ((key = ps2kbd_get()) == 0) {
	}
	return key;
}

// --- Driver state ---
static uint8_t e0_flag = 0;
static uint8_t paused = 0;

// --- IRQ handler ---
void ps2kbd_irq(void)
{
	uint8_t sc = inb(PS2_DATA_PORT);

	if (paused) {
		paused = 0;
		return;
	}

	if (sc == 0xE0) {
		e0_flag = 1;
		return;
	}
	if (sc == 0xE1) {
		paused = 1;
		ps2kbd_push(KEY_PAUSE);
		return;
	}

	uint8_t key = KEY_NONE;
	if (e0_flag) {
		key = sc2key_ext[sc & 0x7F];
		e0_flag = 0;
	} else {
		key = sc2key[sc & 0x7F];
	}

	if (key != KEY_NONE)
		ps2kbd_push(key);

	// send EOI
	outb(0x20, 0x20);
}

// --- Initialization ---
void ps2kbd_init(void)
{
	// enable keyboard port
	outb(PS2_CMD_PORT, 0xAE);
	(void)inb(PS2_DATA_PORT); // flush
	e0_flag = 0;
	paused = 0;
}
#define KS_CAN_MAX 256

// blocking line input from ps2kbd
size_t ps2kbd_kscan_line(char *buf, size_t maxlen)
{
	size_t len = 0;

	while (1) {
		uint8_t key = ps2kbd_kscan(); // wait for next key

		// handle enter
		if (key == KEY_ENTER) {
			buf[len] = '\0'; // null terminate
			break;
		}

		// handle backspace
		else if (key == KEY_BACKSPACE) {
			if (len > 0) {
				len--;
				// optionally: move cursor back on screen
			}
		}

		// normal keys
		else if (len < maxlen - 1) {
			char c = 0;

			switch (key) {
			case KEY_A:
				c = 'a';
				break;
			case KEY_B:
				c = 'b';
				break;
			case KEY_C:
				c = 'c';
				break;
			case KEY_D:
				c = 'd';
				break;
			case KEY_E:
				c = 'e';
				break;
			case KEY_F:
				c = 'f';
				break;
			case KEY_G:
				c = 'g';
				break;
			case KEY_H:
				c = 'h';
				break;
			case KEY_I:
				c = 'i';
				break;
			case KEY_J:
				c = 'j';
				break;
			case KEY_K:
				c = 'k';
				break;
			case KEY_L:
				c = 'l';
				break;
			case KEY_M:
				c = 'm';
				break;
			case KEY_N:
				c = 'n';
				break;
			case KEY_O:
				c = 'o';
				break;
			case KEY_P:
				c = 'p';
				break;
			case KEY_Q:
				c = 'q';
				break;
			case KEY_R:
				c = 'r';
				break;
			case KEY_S:
				c = 's';
				break;
			case KEY_T:
				c = 't';
				break;
			case KEY_U:
				c = 'u';
				break;
			case KEY_V:
				c = 'v';
				break;
			case KEY_W:
				c = 'w';
				break;
			case KEY_X:
				c = 'x';
				break;
			case KEY_Y:
				c = 'y';
				break;
			case KEY_Z:
				c = 'z';
				break;

			case KEY_1:
				c = '1';
				break;
			case KEY_2:
				c = '2';
				break;
			case KEY_3:
				c = '3';
				break;
			case KEY_4:
				c = '4';
				break;
			case KEY_5:
				c = '5';
				break;
			case KEY_6:
				c = '6';
				break;
			case KEY_7:
				c = '7';
				break;
			case KEY_8:
				c = '8';
				break;
			case KEY_9:
				c = '9';
				break;
			case KEY_0:
				c = '0';
				break;

			case KEY_SPACE:
				c = ' ';
				break;
			case KEY_DOT:
				c = '.';
				break;
			case KEY_COMMA:
				c = ',';
				break;
			case KEY_MINUS:
				c = '-';
				break;
			case KEY_SLASH:
				c = '/';
				break;
			case KEY_SEMICOLON:
				c = ';';
				break;
			case KEY_APOSTROPHE:
				c = '\'';
				break;
			case KEY_EQUAL:
				c = '=';
				break;
			case KEY_GRAVE:
				c = '`';
				break;
			case KEY_BACKSLASH:
				c = '\\';
				break;

			default:
				c = 0;
				break;
			}

			if (c)
				buf[len++] = c;
		}
	}

	return len; // chars read (excluding '\0')
}
