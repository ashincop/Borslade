#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
FILE* stdin;
FILE* stdout;
FILE* stderr;
int fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    uint64_t result;
    
    // 1. Declare a local variable tied to the r8 register
    register uint64_t r8_val asm("r8") = (uint64_t)stream;

    asm volatile (
        "int $0x30"
        : "=a"(result)
        : "a"(23),        // rax
          "b"(ptr),       // rbx
          "c"(size),      // rcx
          "d"(nmemb),     // rdx
          "r"(r8_val)     // Forces the compiler to use r8
        :
    );
    stream->error = result;
    return (int)result;
}
// Helper to convert numbers to strings for %d, %x, and %p
static void long_to_str(char *buf, uint64_t n, int base) {
    char tmp[64];
    int i = 0;
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (n > 0) {
        int rem = n % base;
        tmp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        n /= base;
    }
    // Reverse into buf
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

int vfprintf(FILE *stream, const char *fmt, va_list ap) {
    int written = 0;
    char buf[64]; // Buffer for number conversions

    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++; // Skip '%'
            switch (*fmt) {
                case 's': {
                    char *s = va_arg(ap, char *);
                    if (!s) s = "(null)";
                    // Find length of s
                    size_t len = 0;
                    while (s[len]) len++;
                    written += fwrite(s, 1, len, stream);
                    break;
                }
                case 'd': {
                    int d = va_arg(ap, int);
                    if (d < 0) {
                        fwrite("-", 1, 1, stream);
                        d = -d;
                        written++;
                    }
                    long_to_str(buf, (uint64_t)d, 10);
                    size_t len = 0; while(buf[len]) len++;
                    written += fwrite(buf, 1, len, stream);
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(ap, unsigned int);
                    long_to_str(buf, (uint64_t)x, 16);
                    size_t len = 0; while(buf[len]) len++;
                    written += fwrite(buf, 1, len, stream);
                    break;
                }
                case 'p': {
                    void *p = va_arg(ap, void *);
                    fwrite("0x", 1, 2, stream);
                    long_to_str(buf, (uint64_t)p, 16);
                    size_t len = 0; while(buf[len]) len++;
                    written += fwrite(buf, 1, len, stream + 2);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(ap, int);
                    written += fwrite(&c, 1, 1, stream);
                    break;
                }
                case '%': {
                    written += fwrite("%", 1, 1, stream);
                    break;
                }
                default: {
                    // Print the unknown character
                    written += fwrite(fmt, 1, 1, stream);
                    break;
                }
            }
        } else {
            // Literal character
            written += fwrite(fmt, 1, 1, stream);
        }
        fmt++;
    }
    return written;
}
int vprintf(const char *fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}
int fprintf(FILE *stream, const char *fmt, ...) {
    int written = 0;
    va_list args;
    va_start(args, fmt);
    written = vfprintf(stream, fmt, args);
    va_end(args);
    return written;
}
int printf(const char *fmt, ...) {
    int written = 0;
    va_list args;
    va_start(args, fmt);
    written = vfprintf(stdout, fmt, args);
    va_end(args);
    return written;
}
int getchar(void) {return 0;}
int gets(char *buf, size_t size) {return 0;}  /* beware: simple, unsafe */
int putchar(int c) {
    unsigned char ch = (unsigned char)c;
    // We use the address of 'ch' because fwrite needs a pointer
    if (fwrite(&ch, 1, 1, stdout) == 1) {
        return c; // Returns the character written on success
    }
    return -1; // EOF/Error
}
int puts(const char *s) {
    // 1. Calculate length (strlen)
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }

    // 2. Write the string
    if (fwrite(s, 1, len, stdout) != len) {
        return -1;
    }

    // 3. puts always adds a newline
    char newline = '\n';
    if (fwrite(&newline, 1, 1, stdout) != 1) {
        return -1;
    }

    return 0; // Returns non-negative on success
}
int fputc(int c, FILE *stream) {
    unsigned char ch = (unsigned char)c;

    // Use the address of the local variable 'ch' as the pointer for fwrite
    // size = 1, nmemb = 1
    if (fwrite(&ch, 1, 1, stream) == 1) {
        return (int)ch;
    }

    return -1; // Standard return for EOF/Error
}
int fputs(const char *s, FILE *f) {
    while (*s) fputc(*s++, f);
    return 0;
}
long ftell(FILE *stream) {
    // Character devices (like consoles) don't have a position
    return 0; 
}

int fseek(FILE *stream, long offset, int whence) {
    // Seeking is typically not supported on stdout/stderr
    // Return -1 to indicate the operation is not supported
    return -1;
}
int fflush(FILE *stream) {
    return 0;
}
int feof(FILE *stream) {
    return stream->eof;
}
int ferror(FILE *stream) {
    return stream->error;
}
void clearerr(FILE *stream) {
    stream->eof = 0;
    stream->error = 0;
}