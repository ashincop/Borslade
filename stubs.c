#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* --- Syscall Numbers --- */
#define SYS_WRITE         0   /* Hooked to printf/console */
#define SYS_STDIN_READ    2   /* Read from keyboard/stdin */
#define SYS_BRK           7   /* Memory allocation */
#define SYS_OPEN          8   /* VFS Open */
#define SYS_READ          9   /* VFS Read (Files) */
#define SYS_CLOSE        10   /* VFS Close */
#define SYS_LSEEK        11   /* VFS Seek */

/* Standard 3-arg wrapper (RAX=num, RBX=arg1, RCX=arg2, RDX=arg3) */
long __syscall3(long num, long a1, long a2, long a3) {
    long ret;
    __asm__ __volatile__ (
        "int $0x30"
        : "=a"(ret)
        : "a"(num), "b"(a1), "c"(a2), "d"(a3)
        : "memory"
    );
    return ret;
}

/* Specific wrapper for your Stdin Read (RAX=2, RBX=buffer) */
static inline long __syscall_stdin(long buf) {
    long ret;
    __asm__ __volatile__ (
        "int $0x30"
        : "=a"(ret)
        : "a"((long)SYS_STDIN_READ), "b"(buf)
        : "memory"
    );
    return ret;
}
/* Newlib sometimes looks for these names specifically */

int getpid(void) {
    return 1;
}

int kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

/* Keep these for compatibility if other parts of libc use them */
int _getpid(void) { return getpid(); }
int _kill(int pid, int sig) { return kill(pid, sig); }
/* --- Read Stub with Redirect --- */
int read(int file, char *ptr, int len) {
    if (file == 0) {
        /* Use your specific stdin syscall. 
           Since it likely reads one character or a line, 
           we pass the buffer pointer in RBX as requested. */
        return (int)__syscall_stdin((long)ptr);
    }

    /* For TTF files (FD >= 3) */
    if (file < 3) return 0; 

    return (int)__syscall3(SYS_READ, (long)file, (long)ptr, (long)len);
}

/* --- Rest of the stubs remain the same --- */

int write(int file, char *ptr, int len) {
    if (file == 1 || file == 2) {
        return (int)__syscall3(SYS_WRITE, (long)file, (long)ptr, (long)len);
    }
    errno = EBADF;
    return -1;
}

void *sbrk(ptrdiff_t incr) {
    extern char _end;
    static char *heap_end = &_end;
    char *prev_heap_end = heap_end;
    if (__syscall3(SYS_BRK, (long)(heap_end + incr), 0, 0) < 0) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return (void *)prev_heap_end;
}

int open(const char *name, int flags, int mode) {
    return (int)__syscall3(SYS_OPEN, (long)name, (long)flags, (long)mode);
}

int lseek(int file, int ptr, int dir) {
    return (int)__syscall3(SYS_LSEEK, (long)file, (long)ptr, (long)dir);
}

int close(int file) {
    if (file < 3) return 0;
    return (int)__syscall3(SYS_CLOSE, (long)file, 0, 0);
}

void _exit(int status) { while(1) { __asm__("hlt"); } }
int fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int isatty(int file) { return (file >= 0 && file <= 2); }