#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---- memory allocation ---- */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/* ---- process control ---- */
void abort(void) __attribute__((noreturn));
void exit(int status) __attribute__((noreturn));

/* ---- conversions ---- */
int atoi(const char *str);
long atol(const char *str);
long long atoll(const char *str);

long strtol(const char *str, char **endptr, int base);
long long strtoll(const char *str, char **endptr, int base);

/* ---- misc ---- */
int abs(int x);
long labs(long x);
long long llabs(long long x);

/* ---- environment (optional, you can stub) ---- */
char *getenv(const char *name);
int system(const char *command);

#endif /* _STDLIB_H */
