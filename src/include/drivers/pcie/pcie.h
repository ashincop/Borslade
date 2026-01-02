#pragma once
#include <stdint.h>

void detect_pcie_ecam(void);
struct pcie_bars {
    uint64_t bar[6];
    uint8_t bus;
    uint8_t device;
    uint8_t function;
};
void pcie_write32(uintptr_t base_addr, uint8_t bus, uint8_t device, 
                  uint8_t function, uint16_t offset, uint32_t value);
uint32_t pcie_read32(uintptr_t ecam, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset);
// returns pointer to BARs if found, NULL otherwise
struct pcie_bars* pcie_find_bars_ecam_class(uint64_t ecam_base, uint8_t class, uint8_t subclass);
void list_all_pci_devices(void);
const char* pci_lookup_name(uint16_t vendor, uint16_t device);