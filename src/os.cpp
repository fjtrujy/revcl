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

int osTime_080bbd8c;
char *watchdogFlag_080bbd90;

void func_0804ac30(int sig);

int func_0804aaa8()
{
    return ++osTime_080bbd8c;
}

bool OS_Init()
{
    func_0804acb0("OS_Init(Linux)\n");
    osTime_080bbd8c = time(0);
    return true;
}

bool setWatchdog(char *flag, unsigned int seconds)
{
    if (!flag) {
        func_0804ad38("setWatchdog - passed null ptr\n");
        return false;
    }
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    *flag = 0;
    watchdogFlag_080bbd90 = flag;
    signal(SIGALRM, func_0804ac30);
    alarm(seconds);
    return true;
}

bool clearWatchdog()
{
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    watchdogFlag_080bbd90 = 0;
    return true;
}

int func_0804ab6c(const char *name, char *pathOut)
{
    char buf[1024];
    int fd = -1;

    if (!pathOut) {
        func_0804ad38("OS_mkTemp - passed null output ptr\n");
        return -1;
    }
    if (!name) {
        func_0804ad38("OS_mkTemp - passed null input base name\n");
        *pathOut = 0;
        return -1;
    }
    if (strlen(name) > 1023) {
        func_0804ad38("os_mkTemp : an unlikely error has occured\n");
        return -1;
    }
    sprintf(buf, "/tmp/%s.XXXXXX", name);
    fd = mkstemp(buf);
    if (fd < 0) {
        func_0804ad38("failed to make temp in file\n");
        *pathOut = 0;
        return -1;
    }
    mstrcpy(pathOut, buf);
    return fd;
}

int OS_End()
{
    func_0804acb0("OS_End(Linux)\n");
    return 1;
}

void func_0804ac30(int sig)
{
    if (watchdogFlag_080bbd90)
        *watchdogFlag_080bbd90 = 1;
}

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
