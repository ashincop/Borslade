#include <stdio.h>
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
void _init() {
    FILE _stdin;
    FILE _stdout;
    FILE _stderr;
    _stdin.fd = 0;
    _stdin.flags = O_RDONLY;
    _stdin.error = 0;
    _stdout.fd = 1;
    _stdout.flags = O_WRONLY;
    _stdout.error = 0;
    _stderr.fd = 2;
    _stderr.flags = O_WRONLY;
    _stderr.error = 0;
    *stdin  = _stdin;
    *stderr = _stderr;
    *stdout = _stdout;
}
#include <math.h>

double fabs(double x) { return x < 0 ? -x : x; }

double fmod(double x, double y) {
    return x - (long)(x / y) * y;
}

double floor(double x) {
    long i = (long)x;
    return (x < 0 && x != i) ? i - 1 : i;
}

double ceil(double x) {
    long i = (long)x;
    return (x > 0 && x != i) ? i + 1 : i;
}

double trunc(double x) {
    return (long)x;
}

double round(double x) {
    return x >= 0 ? floor(x + 0.5) : ceil(x - 0.5);
}

double sqrt(double x) {
    if (x < 0) return NAN;
    double g = x;
    for (int i = 0; i < 25; i++)
        g = 0.5 * (g + x / g);
    return g;
}

double cbrt(double x) {
    double g = x;
    for (int i = 0; i < 25; i++)
        g = (2.0 * g + x / (g * g)) / 3.0;
    return g;
}

double exp(double x) {
    double sum = 1, term = 1;
    for (int i = 1; i < 30; i++) {
        term *= x / i;
        sum += term;
    }
    return sum;
}

double exp2(double x) {
    return exp(x * 0.6931471805599453);
}

double log(double x) {
    if (x <= 0) return NAN;
    double y = x - 1;
    for (int i = 0; i < 25; i++)
        y -= (exp(y) - x) / exp(y);
    return y;
}

double log10(double x) {
    return log(x) / 2.302585092994046;
}

double log2(double x) {
    return log(x) / 0.6931471805599453;
}

double pow(double a, double b) {
    return exp(b * log(a));
}

double sin(double x) {
    double t = x, s = x;
    for (int i = 1; i < 10; i++) {
        t *= -x * x / ((2*i)*(2*i+1));
        s += t;
    }
    return s;
}

double cos(double x) {
    double t = 1, s = 1;
    for (int i = 1; i < 10; i++) {
        t *= -x * x / ((2*i-1)*(2*i));
        s += t;
    }
    return s;
}

double tan(double x) {
    return sin(x) / cos(x);
}

double atan(double x) {
    double s = x;
    double t = x;
    for (int i = 1; i < 15; i++) {
        t *= -x * x;
        s += t / (2*i + 1);
    }
    return s;
}

double atan2(double y, double x) {
    if (x > 0) return atan(y / x);
    if (x < 0 && y >= 0) return atan(y / x) + PI;
    if (x < 0 && y < 0) return atan(y / x) - PI;
    if (x == 0 && y > 0) return PI / 2;
    if (x == 0 && y < 0) return -PI / 2;
    return 0;
}

double asin(double x) {
    return atan(x / sqrt(1 - x*x));
}

double acos(double x) {
    return PI/2 - asin(x);
}

double sinh(double x) {
    return (exp(x) - exp(-x)) / 2;
}

double cosh(double x) {
    return (exp(x) + exp(-x)) / 2;
}

double tanh(double x) {
    return sinh(x) / cosh(x);
}

double hypot(double x, double y) {
    return sqrt(x*x + y*y);
}

double copysign(double x, double y) {
    return y < 0 ? -fabs(x) : fabs(x);
}
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
#include <stdlib.h>
#include <syscall.h>
void abort(void) {
    for (;;) {}
}

void exit(int status) {
    for (;;) {}
}
typedef struct {
    size_t size;
} malloc_header_t;
size_t malloc_size(void *ptr) {
    if (!ptr) return 0;
    malloc_header_t *h = (malloc_header_t*)ptr - 1;
    return h->size;
}
void *malloc(size_t size) {
    malloc_header_t *h = (malloc_header_t *)__syscall1(7, sizeof(malloc_header_t) + size);
    if (!h) return NULL;
    h->size = size;
    return (void*)(h + 1); // return pointer after header
}
void free(void *ptr) {
    __syscall1(24, (uint64_t)ptr);
}
void* calloc(size_t nmemb, size_t size) {
    void* ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb*size);
    return ptr;
}

void *realloc(void *ptr, size_t new_size) {
    if (!ptr) return malloc(new_size);
    size_t old_size = malloc_size(ptr);
    void *new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    free(ptr);
    return new_ptr;
}
/* --- simple conversions --- */
int atoi(const char *str) {
    int res = 0, sign = 1;
    while (*str == ' ' || *str == '\t') str++;  // skip whitespace
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return sign * res;
}

long atol(const char *str) {
    long res = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return sign * res;
}

long long atoll(const char *str) {
    long long res = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return sign * res;
}

/* --- full conversions with base --- */
long strtol(const char *str, char **endptr, int base) {
    long res = 0;
    int sign = 1;
    if (base == 0) base = 10;

    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    const char *start = str;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'a' && *str <= 'z') digit = *str - 'a' + 10;
        else if (*str >= 'A' && *str <= 'Z') digit = *str - 'A' + 10;
        else break;

        if (digit >= base) break;

        res = res * base + digit;
        str++;
    }

    if (endptr) *endptr = (char *)(str == start ? str : str);
    return sign * res;
}

long long strtoll(const char *str, char **endptr, int base) {
    long long res = 0;
    int sign = 1;
    if (base == 0) base = 10;

    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    const char *start = str;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'a' && *str <= 'z') digit = *str - 'a' + 10;
        else if (*str >= 'A' && *str <= 'Z') digit = *str - 'A' + 10;
        else break;

        if (digit >= base) break;

        res = res * base + digit;
        str++;
    }

    if (endptr) *endptr = (char *)(str == start ? str : str);
    return sign * res;
}

/* --- absolute value helpers --- */
int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }
char *getenv(const char *name) {
    // no env in your OS, so always return NULL
    (void)name;
    return NULL;
}

int system(const char *command) {
    // no shell available, always fail
    (void)command;
    return -1;
}
void _start() {
    printf("Hi ig!\n");
    for(;;);
}