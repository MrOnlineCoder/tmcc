#include <tmcc/error.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void crash(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "tmcc: [fatal] ");
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}