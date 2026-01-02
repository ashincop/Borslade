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
