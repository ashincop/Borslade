#include <stdint.h>

// Direct syscall helper
void sys_print(const char* str) {
    asm volatile (
        "int $0x30"
        :
        : "a"((uint64_t)0), "b"(str)
        : "memory"
    );
}
// Example: Adding two vectors of 4 floats using GCC inline assembly
void sse_add_asm(float* a, float* b, float* result) {
    __asm__ __volatile__ (
        "movups %1, %%xmm0\n\t"    // Move 4 unaligned floats from 'a' to xmm0
        "addps %2, %%xmm0\n\t"     // Add 4 floats from 'b' to xmm0
        "movups %%xmm0, %0\n\t"    // Move result from xmm0 to 'result'
        : "=m" (*result)           // Output operand
        : "m" (*a), "m" (*b)       // Input operands
        : "xmm0"                   // Clobbered registers
    );
}

void _start() {
    sys_print("Hello from C inside a Flat Binarya!\n");
    float result;
    float a = 4.5;
    float b = 4.5;
    sse_add_asm(&a, &b, &result);
    // You can now do logic here
    int x = 10;
    if (x == 10) {
        sys_print("Logic works too.\n");
    }
    for(;;);
}