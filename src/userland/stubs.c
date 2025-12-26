#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>

// --- Existing Syscalls ---
void _init() {}
void _fini() {}
void* _sbrk(int incr) {
    void* result;
    asm volatile ("int $0x30" : "=a"(result) : "a"(7), "b"(incr) : "memory");
    return result;
}

int _write(int file, char *ptr, int len) {
    // Note: It's better to pass the whole buffer to the kernel 
    // rather than looping 1 char at a time for performance.
    asm volatile ("int $0x30" : : "a"(1), "b"(file), "c"(ptr), "d"(len));
    return len;
}

int _read(int file, char* ptr, int len) {
    uint64_t result;
    asm volatile ("int $0x30" : "=a"(result) : "a"(9), "b"((uint64_t)file), "c"(ptr), "d"((uint64_t)len) : "memory");
    return (int)result;
}

// --- New Stubs to fix the TCC linker errors ---

int _open(const char *name, int flags, int mode) {
    // We ignore 'flags' and 'mode' entirely.
    // We just tell the kernel: "Give me this file."
    
    uint64_t fd;
    asm volatile (
        "int $0x30"
        : "=a"(fd) 
        : "a"((uint64_t)8),  // Let's say syscall 8 is 'open'
          "b"(name)          // Just pass the name
    );

    return (int)fd;
}

int _close(int fd) {
    uint64_t result;
    asm volatile (
        "int $0x30"
        : "=a"(result)
        : "a"((uint64_t)10),  /* Syscall 3 */
          "b"((uint64_t)fd)
        : "memory"
    );
    return (int)result;
}

// _lseek(fd, offset, whence)
// Returns int (your kernel's current position)
int _lseek(int fd, int offset, int whence) {
    uint64_t result;
    asm volatile (
        "int $0x30"
        : "=a"(result)
        : "a"((uint64_t)11), /* Syscall 10 */
          "b"((uint64_t)fd),
          "c"((int64_t)offset),
          "d"((uint64_t)whence)
        : "memory"
    );
    return (int)result;
}

int _unlink(const char *name) { return -1; }
int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { return 1; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }

// TCC needs a time source for some internal logic
int _gettimeofday(struct timeval *p, void *z) {
    p->tv_sec = 0;
    p->tv_usec = 0;
    return 0;
}
int gettimeofday(struct timeval *p, void *z) {
    p->tv_sec = 0;
    p->tv_usec = 0;
    return 0;
}
void* sbrk(int incr) {
    void* result;
    asm volatile ("int $0x30" : "=a"(result) : "a"(7), "b"(incr) : "memory");
    return result;
}

int write(int file, char *ptr, int len) {
    // Note: It's better to pass the whole buffer to the kernel 
    // rather than looping 1 char at a time for performance.
    asm volatile ("int $0x30" : : "a"(1), "b"(file), "c"(ptr), "d"(len));
    return len;
}

int read(int file, char* ptr, int len) {
    uint64_t result;
    asm volatile ("int $0x30" : "=a"(result) : "a"(9), "b"((uint64_t)file), "c"(ptr), "d"((uint64_t)len) : "memory");
    return (int)result;
}

// --- New Stubs to fix the TCC linker errors ---

int open(const char *name, int flags, int mode) {
    // We ignore 'flags' and 'mode' entirely.
    // We just tell the kernel: "Give me this file."
    
    uint64_t fd;
    asm volatile (
        "int $0x30"
        : "=a"(fd) 
        : "a"((uint64_t)8),  // Let's say syscall 8 is 'open'
          "b"(name)          // Just pass the name
    );

    return (int)fd;
}

int close(int fd) {
    uint64_t result;
    asm volatile (
        "int $0x30"
        : "=a"(result)
        : "a"((uint64_t)10),  /* Syscall 3 */
          "b"((uint64_t)fd)
        : "memory"
    );
    return (int)result;
}

// _lseek(fd, offset, whence)
// Returns int (your kernel's current position)
int lseek(int fd, int offset, int whence) {
    uint64_t result;
    asm volatile (
        "int $0x30"
        : "=a"(result)
        : "a"((uint64_t)11), /* Syscall 10 */
          "b"((uint64_t)fd),
          "c"((int64_t)offset),
          "d"((uint64_t)whence)
        : "memory"
    );
    return (int)result;
}
int unlink(const char *name) { return -1; }
int fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int isatty(int file) { return 1; }
int getpid(void) { return 1; }
int kill(int pid, int sig) { return -1; }
// --- TCC Dummy Functions ---
// These satisfy references in tcc.c that are used when JIT/Dynamic loading is on
void tcc_run_free(void *s) {}
void tcc_run(void *s) {}
void tcc_set_num_callers(int n) {}
char *getcwd(char *buf, size_t size) { return "/"; }
int execvp(const char *file, char *const argv[]) { return -1; }

// --- Static Dynamic-Loading Stubs ---
void *dlopen(const char *filename, int flag) { return 0; }
void *dlsym(void *handle, const char *symbol) { return 0; }
int dlclose(void *handle) { return 0; }
char *dlerror(void) { return "No dynamic loading"; }

void _exit(int status) { while(1); }