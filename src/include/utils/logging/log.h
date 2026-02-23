#pragma once
#include <stdarg.h>

#define O_ERROR 0
#define O_INFO 1
#define O_OKAY 2
#define O_FATAL 3
#define O_BUG 4
#define O_ALL 5

void log(const char *host, int type, const char *fmt, ...);