void _start() {
    asm volatile("int $0x30" : : "a"(0), "b"("hi from elf"));
    for(;;);
}