#ifndef STRAWC_STDIO_H
#define STRAWC_STDIO_H

#include <stddef.h>   // for size_t, NULL
#include <stdarg.h>   // for va_list
#include <stdint.h>   // for fixed-width types

#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY 0
#define O_WRONLY 1
#define O_BINARY 2
#define O_RDANWR 3

/* --- FILE type --- */
typedef struct FILE {
    int fd;                 // 0 = stdin, 1 = stdout, 2 = stderr
    int flags;              // read/write/error/etc
    int error;              // non‑zero = error happened
    int eof;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* --- basic output --- */
int putchar(int c);
int puts(const char *s);

/* --- basic input --- */
int getchar(void);
int gets(char *buf, size_t size);   /* beware: simple, unsafe */

/* --- file operations --- */
int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *stream, const char *fmt, va_list ap);

/* --- low-level write/read (user-space syscall wrappers) --- */
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* --- flush / seek --- */
int fflush(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

/* --- misc --- */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* STRAWC_STDIO_H */
