#include <arch/x86_64/gdt.h>
void start_kernel() {
    install_gdt();
    for(;;);
}