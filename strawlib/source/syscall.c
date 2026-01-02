#include <syscall.h>

uint64_t __syscall1(uint64_t num, uint64_t arg1) {
    uint64_t ret;
    asm volatile ("int $0x30" :"=a"(ret) : "a"(num), "b"(arg1) :);
    return ret;
}
uint64_t __syscall2(uint64_t num, uint64_t arg1, uint64_t arg2) {
    uint64_t ret;
    asm volatile ("int $0x30" :"=a"(ret) : "a"(num), "b"(arg1), "c"(arg2) :);
    return ret;
}
uint64_t __syscall3(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    asm volatile ("int $0x30" :"=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) :);
    return ret;
}