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

void c_entry() {
    sys_print("Hello from C inside a Flat Binarya!\n");
    
    // You can now do logic here
    int x = 10;
    if (x == 10) {
        sys_print("Logic works too.\n");
    }
}