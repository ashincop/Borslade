#pragma once
#include "types.h"
void idt_install();
#define PORT 0x3f8          // COM1
void sleep(uint32_t ms);