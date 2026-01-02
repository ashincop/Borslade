#include <utils/logging/log.h>
#include <arch/x86_64/idt.h>
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
#include <arch/x86_64/alloc.h>
#include <arch/x86_64/multi.h>
#include <drivers/storage/vfs/vfs.h>
#include <drivers/mouse/mouse.h>
#include <stddef.h>
#include <drivers/pcie/pcie.h>
#include <utils/mem/mem.h>
#include <utils/misc/str.h>
#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#define host "com.strawberry.idt"

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
extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);
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
extern void mouse_wait_input();

extern void mouse_wait_output();
// Send command to mouse
extern void mouse_write(uint8_t cmd);
struct InterruptDescriptor64 idt[256];
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);
void split_64_to_16_16_32(uint64_t input, uint16_t *low, uint16_t *mid, uint32_t *high) {
    // Extract bottom 16 bits (0-15)
    *low  = (uint16_t)(input & 0xFFFF);
    
    // Extract middle 16 bits (16-31)
    *mid  = (uint16_t)((input >> 16) & 0xFFFF);
    
    // Extract top 32 bits (32-63)
    *high = (uint32_t)((input >> 32) & 0xFFFFFFFF);
}

void idt_set_gate(uint8_t index, uint64_t offset, uint8_t attr, uint16_t selector) {
    struct InterruptDescriptor64 entry = {0};
    uint16_t low;
    uint16_t mid;
    uint32_t high;
    split_64_to_16_16_32(offset, &low, &mid, &high);
    entry.ist = 0;
    entry.offset_1 = low;
    entry.offset_2 = mid;
    entry.offset_3 = high;
    entry.selector = selector;
    entry.type_attributes = attr;
    entry.zero = 0;
    idt[index] = entry;
}
#include <uacpi/acpi.h>
struct madt {
    struct acpi_sdt_hdr header; // 36 bytes
    uint32_t lapic_address;     // physical address of local APIC
    uint32_t flags;             // flags, ignore for now
    // entries follow immediately after this
} __attribute__((packed));
struct madt_lapic_entry {
    uint8_t type;   // 0
    uint8_t length; // 8
    uint8_t acpi_processor_id;
    uint8_t lapic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_ioapic_entry {
    uint8_t type;   // 1
    uint8_t length; // 12
    uint8_t id;
    uint8_t reserved;
    uint32_t address;   // MMIO
    uint32_t gsi_base;  // starting GSI
} __attribute__((packed));
void write_ioapic_register(const uintptr_t apic_base, const uint8_t offset, const uint32_t val) 
{
    /* tell IOREGSEL where we want to write to */
    *(volatile uint32_t*)(apic_base) = offset;
    /* write the value to IOWIN */
    *(volatile uint32_t*)(apic_base + 0x10) = val; 
}
 
uint32_t read_ioapic_register(const uintptr_t apic_base, const uint8_t offset)
{
    /* tell IOREGSEL where we want to read from */
    *(volatile uint32_t*)(apic_base) = offset;
    /* return the data from IOWIN */
    return *(volatile uint32_t*)(apic_base + 0x10);
}
enum DeliveryMode {
    EDGE,
    LEVEL
};
enum DestinationMode {
    PHYSICAL,
    LOGICAL
};

void ioapic() {
    uacpi_table madtTbl;
    uacpi_table_find_by_signature("APIC", &madtTbl);
    struct acpi_sdt_hdr *hdr = (struct acpi_sdt_hdr *)madtTbl.ptr;

    // check signature first
    if (memcmp(hdr->signature, "APIC", 4) != 0) return; // not a MADT

    struct madt *madt = (struct madt *)hdr;

    // entries start immediately after fixed MADT fields
    uint8_t *entry = (uint8_t *)madt + sizeof(struct madt);
    uint8_t *end   = (uint8_t *)madt + madt->header.length;

    while (entry < end) {
        uint8_t type = entry[0];
        uint8_t len  = entry[1];

        if (type == 1) {
            struct madt_ioapic_entry *io = (struct madt_ioapic_entry *)entry;
            uint32_t mmio = io->address;

        }

        entry += len;
    }

}

void idtc(uint64_t *stack_anchor) {
    asm volatile("cli");
    // FIXED: Include int_no from ISR macro push (index 20)
    uint64_t rax     = stack_anchor[0];
    uint64_t rbx     = stack_anchor[1];
    uint64_t rcx     = stack_anchor[2];
    uint64_t rdx     = stack_anchor[3];
    uint64_t rbp_    = stack_anchor[4];
    uint64_t rdi     = stack_anchor[5];
    uint64_t rsi     = stack_anchor[6];
    uint64_t r8      = stack_anchor[7];
    uint64_t r9      = stack_anchor[8];
    uint64_t r10     = stack_anchor[9];
    uint64_t r11     = stack_anchor[10];
    uint64_t r12     = stack_anchor[11];
    uint64_t r13     = stack_anchor[12];
    uint64_t r14     = stack_anchor[13];
    uint64_t r15     = stack_anchor[14];

    uint64_t rip     = stack_anchor[15];
    uint64_t cs      = stack_anchor[16];
    uint64_t rflags  = stack_anchor[17];
    uint64_t rsp     = stack_anchor[18];
    uint64_t ss      = stack_anchor[19];

    uint64_t int_no  = stack_anchor[20];

    kprintf("\npanic(%d)\n", int_no);
    
    kprintf("RAX: %x  RBX: %x  RCX: %x  RDX: %x  RBP: %x\n", rax, rbx, rcx, rdx, rbp_);
    kprintf("RDI: %x  RSI: %x\n", rdi, rsi);

    kprintf("RIP:        %x\n", rip);
    kprintf("CS:         %x\n", cs);
    kprintf("RFLAGS:     %x\n", rflags);
    kprintf("RSP:        %x\n", rsp);
    kprintf("SS:         %x\n", ss);
    for(;;);
}

volatile uint64_t ticks = 0;
void irq0c(uint64_t *stack_anchor) {
    ticks++;

}
static inline int interrupts_enabled() {
    unsigned long flags;
    // Push flags to stack, pop into 'flags' variable
    asm volatile("pushf ; pop %0" : "=g"(flags));
    return (flags & 0x200); // Check bit 9
}
void sleep(uint32_t ms) {
    int wenabled = 0;
    if (!interrupts_enabled()) {wenabled = 0; asm("sti");} else {wenabled = 1;}
    uint32_t end_at = ticks + ms;

    while (ticks < end_at) {
        while (ticks < end_at) {
            __asm__ volatile("pause" ::: "memory");
        }

    }
    if (!wenabled) asm("cli");
}

void irq1c(uint64_t *stack_anchor) {
    keyboard_handler(stack_anchor);
}

struct MouseState {
    int8_t dx;
    int8_t dy;
    uint8_t buttons; // bit 0 = left, bit 1 = right, bit 2 = middle
};


void irq12c(uint64_t *stack_anchor) {
    uint8_t data = inb(PS2_DATA);
    kprintf("we got data heyyy!\n");    
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
}
int isdigit(char c) {
    return c >= '0' && c <= '9';
}
int isalpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
int isalnum(char c) {
    return isalpha(c) || isdigit(c);
}
typedef struct kFILE {
    int fd;                 // 0 = stdin, 1 = stdout, 2 = stderr
    int flags;              // read/write/error/etc
    size_t pos;             // cursor / offset (optional but nice)
    int error;              // non‑zero = error happened
} kFILE;
extern uintptr_t g_ecam_base; 
void idtcs(uint64_t *stack_anchor) {
    uint64_t rax     = stack_anchor[ 0];
    uint64_t rbx     = stack_anchor[ 1];
    uint64_t rcx     = stack_anchor[ 2];
    uint64_t rdx     = stack_anchor[ 3];
    uint64_t rbp_    = stack_anchor[ 4];
    uint64_t rdi     = stack_anchor[ 5];
    uint64_t rsi     = stack_anchor[ 6];
    uint64_t r8      = stack_anchor[ 7];
    uint64_t r9      = stack_anchor[ 8];
    uint64_t r10     = stack_anchor[ 9];
    uint64_t r11     = stack_anchor[10];
    uint64_t r12     = stack_anchor[11];
    uint64_t r13     = stack_anchor[12];
    uint64_t r14     = stack_anchor[13];
    uint64_t r15     = stack_anchor[14];

    // Hardware frame (offsets 120-152 = indices 15-19)
    uint64_t rip     = stack_anchor[15];
    uint64_t cs      = stack_anchor[16];
    uint64_t rflags  = stack_anchor[17];
    uint64_t rsp     = stack_anchor[18];
    uint64_t ss      = stack_anchor[19];

    // ISR frame (offset 160 = index 20) ← ADDED!
    uint64_t int_no  = stack_anchor[20];
    if (rax == 0) {
        kprintf((char*)rbx);
        
    } else if (rax == 1) {
        clear_screen((uint32_t)rbx);
        
    } else if (rax == 2) {
        kprintf("[com.strawberry.core.interrupts] RAX=2.\n");
        asm volatile("sti");
        kscan((char *)rbx);
        asm volatile("cli");
        
    } else if (rax == 3) {
        spawn_user_task(rbx, (int)rcx, (char *)rdx, (int)r8);
        log(host, O_INFO, "Spawned PID %d.\n", rcx);
        
    } else if (rax == 4) {
        sleep(rbx);
    } else if (rax == 5) {
        int *pids = get_rpids();
        char **names = get_rnames();
        kprintf("Currently running processes: ");
    
        for (uint64_t i = 0; i < 256; i++) { if (!(names[i] == "")) { kprintf("%s ", names[i]); } }
        kprintf("\n");
    } else if (rax == 6) {
        char* filename = (char*)rbx;
        void* user_dest = (void*)rcx; // Buffer passed from app
        uint32_t size   = (uint32_t)rdx;

        // 1. Perform the actual read
        vfs_read(filename, user_dest, size);

        // 2. Force the return value into RAX manually
        // We use "a" to tell the compiler to put 'user_dest' into RAX
        asm volatile ("" : : "a"(user_dest) :);

    } else if (rax == 7) {
        void* alloc = kmalloc(rbx);
        asm volatile ("" : : "a"((uint64_t)alloc) :);
    } else if (rax == 8) {
        asm volatile ("" : : "a"((uint64_t)sys_open((char*)rbx)) :);
    } else if (rax == 9) {
        asm volatile ("" : : "a"((uint64_t)sys_read((int)rbx, (char *)rcx, (uint32_t)rdx)));
    } else if (rax == 10) {
        asm volatile ("" : : "a"((uint64_t)sys_close((int)rbx)));
    } else if (rax == 11) {
        asm volatile ("" : : "a"((uint64_t)sys_lseek((int)rbx, (int)rcx, (int)rdx)));
    } else if (rax == 12) {
        char* user_filename = (char*)rbx;

        asm volatile ("" : : "a"((uint64_t)vfs_get_filesize(user_filename)) :);
    } else if (rax == 13) {
        asm volatile ("" : : "a"((uint64_t)isdigit((char)rbx)) :);
    } else if (rax == 14) {
        asm volatile ("" : : "a"((uint64_t)isalpha((char)rbx)) :);
    } else if (rax == 15) {
        asm volatile ("" : : "a"((uint64_t)isalnum((char)rbx)) :);
    } else if (rax == 16) {
        asm volatile ("" : : "a"((uint64_t)strcmp((char*)rbx, (char*)rcx)) :);
    } else if (rax == 17) {
        asm volatile ("" : : "a"((uint64_t)memcpy((void*)rbx, (void*)rcx, (size_t)rdx)) :);
    } else if (rax == 18) {
        asm volatile ("" : : "a"((uint64_t)memset((void*)rbx, (int)rcx, (size_t)rdx)) :); 
    } else if (rax == 19) {
        draw_pixel((int)rbx, (int)rcx, (uint32_t)rdx);
    } else if (rax == 20) {
        // void pcie_write32(uintptr_t base_addr, uint8_t bus, uint8_t device, 
        //           uint8_t function, uint16_t offset, uint32_t value)
        pcie_write32(g_ecam_base, (uint8_t)rbx, (uint8_t)rcx, (uint8_t)rdx, (uint16_t)r8, (uint32_t)r9);
    } else if (rax == 21) {
        // uint32_t pcie_read32(uintptr_t ecam, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset)
        asm volatile ("" : : "a"((uint64_t)pcie_read32(g_ecam_base, (uint8_t)rbx, (uint8_t)rcx, (uint8_t)rdx, (uint16_t)r8)) :);
    } else if (rax == 22) {
        // const char* pci_lookup_name(uint16_t vendor, uint16_t device)
        asm volatile ("" : : "a"((uint64_t)pci_lookup_name((uint16_t)rbx, (uint16_t)rcx)) :);
    } else if (rax == 23) {
        // Calculate total bytes from the fwrite parameters
        uint64_t total_bytes = rcx * rdx;
        char *buffer = (char *)rbx;
        kFILE *file = (kFILE *)r8;

        if (file != (void*)0 && file->fd != 0) {
            if (file->fd == 1) {
                // stdout logic
                for (uint64_t i = 0; i < total_bytes; i++) {
                    char s[2] = {buffer[i], 0};
                    kprint_s(s);
                }
                // Write 0 to rax to signal success
                asm volatile ("" : : "a"((uint64_t)0) :);
            } 
            else if (file->fd == 2) {
                // stderr logic (Red)
                extern uint32_t fg_color;
                uint32_t lastcolor = fg_color;
                fg_color = 0xFF0000;

                for (uint64_t i = 0; i < total_bytes; i++) {
                    char s[2] = {buffer[i], 0};
                    kprint_s(s);
                }

                fg_color = lastcolor;
                // Write 0 to rax to signal success
                asm volatile ("" : : "a"((uint64_t)0) :);
            } 
            else if (file->fd > 3) {
                // Custom FD logic or error
                asm volatile ("" : : "a"((uint64_t)2) :);
            }
        } else {
            // Error: Null file or invalid FD
            asm volatile ("" : : "a"((uint64_t)1) :);
        }
    } else if (rax == 24) {
        kfree((void *)rbx);
    }
}
void init_timer(uint32_t frequency) {   
    // 1193182 is the base frequency of the PIT
    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);             // Command port: Square wave mode
    outb(0x40, (uint8_t)(divisor & 0xFF));        // Low byte
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); // High byte
}
#define EXTERN_ISR(n) extern void isr##n(void);

#define GENERATE_EXTERN_ISRS() \
    EXTERN_ISR(0)  EXTERN_ISR(1)  EXTERN_ISR(2)  EXTERN_ISR(3)  EXTERN_ISR(4)  \
    EXTERN_ISR(5)  EXTERN_ISR(6)  EXTERN_ISR(7)  EXTERN_ISR(8)  EXTERN_ISR(9)  \
    EXTERN_ISR(10) EXTERN_ISR(11) EXTERN_ISR(12) EXTERN_ISR(13) EXTERN_ISR(14) \
    EXTERN_ISR(15) EXTERN_ISR(16) EXTERN_ISR(17) EXTERN_ISR(18) EXTERN_ISR(19) \
    EXTERN_ISR(20) EXTERN_ISR(21) EXTERN_ISR(22) EXTERN_ISR(23) EXTERN_ISR(24) \
    EXTERN_ISR(25) EXTERN_ISR(26) EXTERN_ISR(27) EXTERN_ISR(28) EXTERN_ISR(29) \
    EXTERN_ISR(30) EXTERN_ISR(31)

GENERATE_EXTERN_ISRS()

extern void idtstubs();
extern void irq0();
extern void irq1();
extern void irq12();
extern void pic_remap();
void idt_install() {
    __asm__ volatile ("cli");
    void* isr_table[32] = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,  isr8,  isr9,
        isr10, isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19,
        isr20, isr21, isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29,
        isr30, isr31
    };

    uint8_t isr_flags[32] = {
        0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0xEE, 0x8E,  // 0-9
        0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0x8E,  // 10-15
        0x8E, 0xEE, 0x8E, 0x8E,  // 16-19
        0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E, 0x8E,  // 20-27
        0x8E, 0x8E, 0x8E, 0x8E   // 28-31
    };

    // ONE LINE replaces loop:
    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t)isr_table[i], isr_flags[i], 0x08);
    }
    idt_set_gate(48, (uint64_t)&idtstubs, 0xEE, 0x08);
    idt_set_gate(32, (uint64_t)irq0, 0xEE, 0x08);
    idt_set_gate(33, (uint64_t)irq1, 0xEE, 0x08);
    idt_set_gate(44, (uint64_t)irq12, 0xEE, 0x08);
    struct idtr idtp;
    idtp.offset = (uint64_t)&idt;
    idtp.size = sizeof(idt)-1;
    init_timer(1000);
    pic_remap();
    keyboard_init();
    __asm__ volatile ("lidt %0" : : "m"(idtp));
    __asm__ volatile ("sti");
}