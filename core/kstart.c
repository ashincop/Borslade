#include "arch_i386/gdt.h"
void start_kernel() {
    gdt_install();
    for(;;);
}