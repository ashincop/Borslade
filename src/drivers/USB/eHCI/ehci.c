#include <drivers/pcie/pcie.h>
#include <drivers/screen/fb.h>
#include <stdint.h>
#include <stddef.h>

void init_xhci() {
    struct pcie_bars* bars = pcie_find_bars_ecam_class(0xE0000000, 0x0C, 0x03);
    if (!bars) return;

    pcie_write32(0xE0000000, bars->bus, bars->device, 0, 0x04, 0x07);

    uintptr_t cap_base = (uintptr_t)bars->bar[0];
    uintptr_t op_base = cap_base + *(volatile uint8_t*)cap_base;
    uintptr_t db_base = cap_base + (*(volatile uint32_t*)(cap_base + 0x14) & ~0x3);
    uintptr_t rt_base = cap_base + (*(volatile uint32_t*)(cap_base + 0x18) & ~0x1F);

    // 1. Reset
    *(volatile uint32_t*)op_base &= ~1; 
    while(!(*(volatile uint32_t*)(op_base + 0x04) & 1));
    *(volatile uint32_t*)op_base |= 2;  
    while(*(volatile uint32_t*)op_base & 2);

    // 2. CONFIG: Set Max Device Slots
    // HCSPARAMS1 (cap_base + 0x04) tells us the max slots available.
    uint32_t max_slots = *(volatile uint32_t*)(cap_base + 0x04) & 0xFF;
    *(volatile uint32_t*)(op_base + 0x38) = max_slots; // Set MaxSlotsEn

    // 3. Clear Memory (0x80000 block)
    for(int i = 0; i < 4096; i++) { ((uint32_t*)0x80000)[i] = 0; }

    // 4. Setup Command Ring
    *(volatile uint32_t*)(op_base + 0x1C) = 0;
    *(volatile uint32_t*)(op_base + 0x18) = 0x80000 | 1; 

    // 5. Setup ERST (Segment Table) at 0x82000
    uint32_t* erst = (uint32_t*)0x82000;
    erst[0] = 0x81000; erst[1] = 0; erst[2] = 256; erst[3] = 0;

    // 6. Runtime Offsets (Standard xHCI)
    // 0x20: IMAN, 0x28: ERSTSZ, 0x30: ERSTBA, 0x38: ERDP
    volatile uint32_t* iman   = (volatile uint32_t*)(rt_base + 0x20);
    volatile uint32_t* erstsz = (volatile uint32_t*)(rt_base + 0x28);
    volatile uint32_t* erstba = (volatile uint32_t*)(rt_base + 0x30);
    volatile uint32_t* erdp   = (volatile uint32_t*)(rt_base + 0x38);

    *iman = 2;              // Interrupt Enable
    *erstsz = 1;            // 1 Segment
    *(erstba + 1) = 0;      // High 32 bits
    *erstba = 0x82000;      // Low 32 bits
    *(erdp + 1) = 0;        // High 32 bits
    *erdp = 0x81000 | 8;    // Low 32 bits + EHB (Event Handler Busy)

    // 7. Start
    *(volatile uint32_t*)op_base |= 1;
    while(*(volatile uint32_t*)(op_base + 0x04) & 1);

    // 8. Command: No-Op (Type 23)
    uint32_t* cmd = (uint32_t*)0x80000;
    cmd[0] = 0; cmd[1] = 0; cmd[2] = 0;
    cmd[3] = (23 << 10) | 1; 

    __asm__ volatile ("mfence" ::: "memory");
    __asm__ volatile ("clflush (%0)" : : "r"(cmd) : "memory");

    // 9. Kick Doorbell 0
    *(volatile uint32_t*)db_base = 0; 

    while(1) {
        uint32_t current_erdp = *erdp & ~0xF;
        uint32_t* ev = (uint32_t*)0x81000;
        __asm__ volatile ("clflush (%0)" : : "r"(ev) : "memory");

        if (current_erdp != 0x81000 || ev[3] != 0) {
            kprintf("\n*** SUCCESS! DOORBELL HEARD ***\n");
            kprintf("ERDP: %x | Event Word 3: %x\n", current_erdp, ev[3]);
            break;
        }

        static int t = 0;
        if (t++ % 2000000 == 0) {
            kprintf("IDLE. STS:%x ERDP_REG:%x\n", *(volatile uint32_t*)(op_base + 0x04), *erdp);
        }
    }
}