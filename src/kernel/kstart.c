#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/multi.h>
#include <arch/x86_64/alloc.h>
#include <drivers/screen/fb.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/storage/vfs/vfs.h>
#include <drivers/pcie/pcie.h>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <uacpi/tables.h>
#include <stdint.h>
#include <drivers/USB/eHCI/ehci.h>
#include <utils/misc/inline.h>
#include <utils/logging/log.h>

#define host "com.strawberry.kernel.core"

uintptr_t g_ecam_base = 0xE0000000; 


// A simple square wave buffer (must be in physical memory)
static uint16_t beep_samples[1024] __attribute__((aligned(4)));

// Buffer Descriptor List entry
struct ac97_bdl {
    uint32_t addr;   // Physical address of samples
    uint16_t length; // Sample count
    uint16_t flags;  // Bit 15: Interrupt on Completion (sets status bit)
} __attribute__((packed));

static struct ac97_bdl bdl_entry __attribute__((aligned(16)));

void beep(uintptr_t ecam_base) {
    uint16_t nambar = 0, nabmbar = 0;
    int found = 0;

    // 1. SCAN PCI (Same as your code)
    for (int b = 0; b < 256 && !found; b++) {
        for (int d = 0; d < 32 && !found; d++) {
            uint32_t id = pcie_read32(ecam_base, b, d, 0, 0x00);
            if ((id & 0xFFFF) == 0xFFFF) continue;
            uint32_t class_info = pcie_read32(ecam_base, b, d, 0, 0x08);
            if ((class_info >> 16) == 0x0401) {
                nambar = pcie_read32(ecam_base, b, d, 0, 0x10) & ~0x1;
                nabmbar = pcie_read32(ecam_base, b, d, 0, 0x14) & ~0x1;
                found = 1;
                uint32_t cmd = pcie_read32(ecam_base, b, d, 0, 0x04);
                pcie_write32(ecam_base, b, d, 0, 0x04, cmd | 0x5); 
            }
        }
    }
    if (!found) return;

    // 2. INITIALIZE AUDIO
    outw(nambar + 0x02, 0x0000); // Unmute Master
    outw(nambar + 0x18, 0x0000); // Unmute PCM Out

    // MAKE IT DEEP: i % 256 creates a much lower frequency (approx 187Hz at 48kHz)
    // We use a larger buffer (32768 samples) for smoother polling
    #define BEEP_SIZE 32768/2
    static uint16_t samples[BEEP_SIZE] __attribute__((aligned(4096)));
    for (int i = 0; i < BEEP_SIZE/2; i++) 
        samples[i] = (i % 512 < 128) ? 0x2000 : 0xE000;
    for (int i = BEEP_SIZE/2; i < BEEP_SIZE; i++) 
        samples[i] = (i % 16 < 8) ? 0x2000 : 0xE000;

    // 3. SETUP BDL
    bdl_entry.addr = (uint32_t)(uintptr_t)samples; 
    bdl_entry.length = BEEP_SIZE; 
    bdl_entry.flags = (1 << 15); // IOC bit

    // 4. PLAYBACK LOOP (4 SECONDS)
    // At 48000 samples/sec, 4 seconds is ~192,000 samples.
    // Our buffer is 32,768, so we repeat it 6 times.
    for (int repeat = 0; repeat < 2; repeat++) {
        outb(nabmbar + 0x1B, 0x02); // Reset Channel
        outl(nabmbar + 0x10, (uint32_t)(uintptr_t)&bdl_entry);
        outb(nabmbar + 0x15, 0);    // LVI = 0
        outb(nabmbar + 0x1B, 0x01); // Start

        // POLL until this 0.68s chunk is done
        while (!(inw(nabmbar + 0x16) & (1 << 1))) {
            __asm__ volatile ("pause");
        }
        
        // Clear status for next iteration
        outw(nabmbar + 0x16, 0x1E);
    }

    // 5. CLEANUP
    outb(nabmbar + 0x1B, 0x00); // Stop
}

void enable_sse() {
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear EM (Coprocessor Emulation)
    cr0 |= (1 << 1);  // Set MP (Monitor Coprocessor)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  // Set OSFXSR (FXSAVE/FXRSTOR support)
    cr4 |= (1 << 10); // Set OSXMMEXCPT (SIMD Exception support)
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}
uint8_t user_stack[(4096*9)];
uint64_t probe_memory_size() {
    uint64_t last_accessible_addr = 0;
    // Start probing every 1MB starting after the kernel
    for (uint64_t addr = 0x1000000; addr < 0xFFFFFFFF; addr += 0x100000) {
        volatile uint64_t* ptr = (uint64_t*)addr;
        uint64_t backup = *ptr;
        *ptr = 0xDEADBEEF;
        if (*ptr == 0xDEADBEEF) {
            *ptr = backup;
            last_accessible_addr = addr;
        } else {
            break;
        }
    }
    return last_accessible_addr;
}
uint64_t g_mbi_ptr;

struct mcfg_entry {
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed));

void uacpi_level2_dependent() {
    uacpi_table madtTbl;
    int ret = uacpi_table_find_by_signature("APIC", &madtTbl);
    kprintf("APIC AT %x\n", madtTbl.ptr);
}

void start_kernel(uint64_t mbi_addr) {
    g_mbi_ptr = mbi_addr;
    init_gop(mbi_addr);
    init_serial();
    init_alloc(0x20000000, mbi_addr);  
    log(host, O_OKAY, "allocation initialized.\n");
    uacpi_status st = uacpi_initialize(0);
    if (st != UACPI_STATUS_OK) {
        kprintf("uACPI Init Failed: %s\n", uacpi_status_to_string(st));
        for(;;);
    }
    st = uacpi_namespace_load();
    if (st != UACPI_STATUS_OK) {
        kprintf("Namespace Load Failed: %s\n", uacpi_status_to_string(st));
        for(;;);
        // If this fails, find_by_signature for MCFG will fail too!
    }
    uacpi_level2_dependent();
    st = uacpi_namespace_initialize();
    if (st != UACPI_STATUS_OK) {
        kprintf("Namespace Init Failed: %s\n", uacpi_status_to_string(st));
        for(;;);
    }    
    install_gdt();
    idt_install();
    log(host, O_OKAY, "Initialized IDT!\n");
    char buf[32];
    enable_sse();
    init_vfs(mbi_addr);
    list_all_pci_devices();
    __asm__ volatile ("cli");
    init_multitasking();
    __asm__ volatile ("sti");
    for(;;);
}   