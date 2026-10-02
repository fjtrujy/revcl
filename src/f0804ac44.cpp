// OS layer for Linux (CIE group 1, after preprocess.cpp).

#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "os.h"

extern bool flag_080b3038;
extern bool quiet_080bbd98;
extern int errorOccurred;
extern char *outName_080b303c;


void func_0804ac44()
{
    flag_080b3038 = 1;
}

int func_0804ac50(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    return vfprintf(stderr, fmt, ap);
}

int func_0804ac7c(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "WARN: ");
    return vfprintf(stderr, fmt, ap);
}

int func_0804acb0(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO: ");
    return vfprintf(stderr, fmt, ap);
}

int func_0804acf4(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO2: ");
    return vfprintf(stderr, fmt, ap);
}

int func_0804ad38(const char *fmt, ...)
{
    if (flag_080b3038)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    errorOccurred = 1;
    fprintf(stderr, "ERROR: ");
    return vfprintf(stderr, fmt, ap);
}

int func_0804ad84(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    flag_080b3038 = 1;
    fprintf(stderr, "USER: ");
    return vfprintf(stderr, fmt, ap);
}

int func_0804adbc(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    return vsprintf(buf, fmt, ap);
}

int func_0804add4(const char *s)
{
    return atoi(s);
}

void func_0804adf0(int status)
{
    if (status) {
        if (outName_080b303c)
            printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", outName_080b303c);
        else
            puts("ERROR: something went wrong, vcl's outta here.");
    }
    OS_End();
    exit(status);
}

void func_0804ae34(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (!flag_080b3038) {
        errorOccurred = 1;
        fprintf(stderr, "ERROR: ");
        vfprintf(stderr, fmt, ap);
    }
    if (outName_080b303c)
        printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", outName_080b303c);
    else
        puts("ERROR: something went wrong, vcl's outta here.");
    OS_End();
    exit(-1);
}

void func_0804aeb0()
{
    if (outName_080b303c)
        printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", outName_080b303c);
    else
        puts("ERROR: something went wrong, vcl's outta here.");
    OS_End();
    exit(-1);
}

char *func_0804aef0(const char *path, int *len, bool nulTerm)
{
    bool ok = false;
    char *buf = 0;
    int size = -1;
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        printf("cgReadFile: failed to open file '%s'\n", path);
    } else {
        size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        int allocSize = size;
        if (nulTerm)
            allocSize = size + 1;
        buf = (char *)malloc(allocSize);
        if (buf) {
            if (read(fd, buf, size) == size)
                ok = true;
            else
                printf("cgReadFile: failed to read all of file %s\n", path);
        }
        if (nulTerm)
            buf[size] = 0;
    }
    if (fd != -1)
        close(fd);
    if (!ok) {
        size = -1;
        if (buf)
            free(buf);
        buf = 0;
    }
    if (len)
        *len = size;
    return buf;
}

char *func_0804aff0(int fd, int *len, bool nulTerm)
{
    int ok = 0;
    int used = 0;
    char *buf = (char *)malloc(0x10000);

    if (!buf) {
        func_0804ad38("failed to alloc initial read buffer\n");
    } else {
        int size = 0x10000;
        for (;;) {
            if (size - used < 0x10000) {
                size += 0x10000;
                char *nbuf = (char *)realloc(buf, size);
                if (!nbuf) {
                    func_0804ad38("nReadFile - failed to extend buffer\n");
                    break;
                }
                buf = nbuf;
            }
            int n = read(fd, buf + used, 0xffff);
            if (n < 0) {
                func_0804ad38("read returned <0\n");
                break;
            }
            if (n == 0) {
                ok = 1;
                break;
            }
            used += n;
        }
        if (nulTerm && buf)
            buf[used] = 0;
    }
    if (!ok && buf) {
        free(buf);
        buf = 0;
        used = 0;
    }
    if (fd > 0)
        close(fd);
    if (len)
        *len = used;
    return buf;
}
