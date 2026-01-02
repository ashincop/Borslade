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