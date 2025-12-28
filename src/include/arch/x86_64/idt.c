#include "idt.h"
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
#include "alloc.h"
#include <arch/x86_64/multi.h>
#include <drivers/storage/vfs/vfs.h>
struct InterruptDescriptor64 idt[256];
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
struct Registers {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
#define PORT 0x3f8          // COM1

int init_serial() {
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (38400 baud)
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   
   // Check if hardware is actually there (Loopback test)
   outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + 0, 0xAE);    // Test data
   if(inb(PORT + 0) != 0xAE) return 1;
   
   outb(PORT + 4, 0x0F);    // Set normal operation mode
   return 0;
}
int is_transmit_empty() {
   return inb(PORT + 5) & 0x20;
}
 
void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outb(PORT, a);
}

void kprint_serial(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}
void idtc(uint64_t *stack_anchor) {
    asm volatile("cli");
    // FIXED: Include int_no from ISR macro push (index 20)
    uint64_t rax     = stack_anchor[-13];
    uint64_t rbx     = stack_anchor[-12];
    uint64_t rcx     = stack_anchor[-11];
    uint64_t rdx     = stack_anchor[-10];
    uint64_t rbp_    = stack_anchor[-9];
    uint64_t rdi     = stack_anchor[-8];
    uint64_t rsi     = stack_anchor[-7];
    uint64_t r8      = stack_anchor[-6];
    uint64_t r9      = stack_anchor[-5];
    uint64_t r10     = stack_anchor[-4];
    uint64_t r11     = stack_anchor[-3];
    uint64_t r12     = stack_anchor[-2];
    uint64_t r13     = stack_anchor[-1];
    uint64_t r14     = stack_anchor[0];
    uint64_t r15     = stack_anchor[1];

    // Hardware frame (offsets 120-152 = indices 15-19)
    uint64_t rip     = stack_anchor[2];
    uint64_t cs      = stack_anchor[3];
    uint64_t rflags  = stack_anchor[4];
    uint64_t rsp     = stack_anchor[5];
    uint64_t ss      = stack_anchor[6];

    // ISR frame (offset 160 = index 20) ← ADDED!
    uint64_t int_no  = stack_anchor[-15];
    const char* exception_names[32] = {
    "Divide Error",           // 0
    "Debug Exception",        // 1  
    "Non-Maskable Interrupt", // 2
    "Breakpoint",             // 3
    "Overflow",               // 4
    "Bound Range Exceeded",   // 5
    "Invalid Opcode",         // 6
    "Device Not Available",   // 7
    "Double Fault",           // 8
    "Coprocessor Segment Overrun", // 9
    "Invalid TSS",            // 10
    "Segment Not Present",    // 11
    "Stack Segment Fault",    // 12
    "General Protection",     // 13 ← YOUR CRASH!
    "Page Fault",             // 14
    "Reserved",               // 15
    "x87 FPU Floating-Point Error", // 16
    "Alignment Check",        // 17
    "Machine Check",          // 18
    "SIMD Floating-Point Exception", // 19
    "Virtualization Exception", // 20
    "Control Protection Exception", // 21
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
};


    kprintf("\npanic(%d)\n", int_no);
    
    // 1. Check the Registers we pushed
    kprintf("RAX: %x  RBX: %x  RCX: %x  RDX: %x  RBP: %x\n", rax, rbx, rcx, rdx, rbp_);
    kprintf("RDI: %x  RSI: %x\n", rdi, rsi);


    // 3. Check the Trap Frame (The "Ticket" back to Ring 3)
    // If these look like memory addresses instead of 0x1B/0x23, your struct is wrong!
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
void sleep(uint64_t ms) {
    // Calculate when we should wake up
    // This assumes your Timer is set to 1000Hz (1ms per tick)
    uint64_t end = ticks + ms;

    while (ticks < end) {
        // 'hlt' stops the CPU execution until an interrupt fires.
        // When IRQ0 fires, 'ticks' increments, the CPU wakes up,
        // checks the 'while' condition, and if not done, 'hlt' again.
        asm volatile("hlt"); 
    }
}
void irq1c(uint64_t *stack_anchor) {
    keyboard_handler(stack_anchor);
}
void irq12c(uint64_t *stack_anchor) {

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
int strcmp(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return *a - *b;
}

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
        kprintf("[com.strawberry.core.userland] %s", (char*)rbx);
        
    } else if (rax == 1) {
        clear_screen((uint32_t)rbx);
        
    } else if (rax == 2) {
        kprintf("[com.strawberry.core.interrupts] RAX=2.\n");
        asm volatile("sti");
        kscan((char *)rbx);
        asm volatile("cli");
        
    } else if (rax == 3) {
        spawn_user_task(rbx, (int)rcx, (char *)rdx, (int)r8);
        kprintf("[com.strawberry.core.userland] Spawned PID %d.\n", rcx);
        
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
        asm volatile ("" : : "a"((uint64_t)isdigit((char)rbx)));
    } else if (rax == 14) {
        asm volatile ("" : : "a"((uint64_t)isalpha((char)rbx)));
    } else if (rax == 15) {
        asm volatile ("" : : "a"((uint64_t)isalnum((char)rbx)));
    } else if (rax == 16) {
        asm volatile ("" : : "a"((uint64_t)strcmp((char*)rbx, (char*)rcx)));
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