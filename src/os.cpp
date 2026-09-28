// OS layer for Linux (CIE group 1, after preprocess.cpp).

#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "os.h"

extern bool flag_080b3038;
extern bool quiet_080bbd98;

int osTime_080bbd8c;
char *watchdogFlag_080bbd90;

void func_0804ac30(int sig);

int func_0804aaa8()
{
    return ++osTime_080bbd8c;
}

int OS_Init()
{
    func_0804acb0("OS_Init(Linux)\n");
    osTime_080bbd8c = time(0);
    return 1;
}

int setWatchdog(char *flag, unsigned int seconds)
{
    if (!flag) {
        func_0804ad38("setWatchdog - passed null ptr\n");
        return 0;
    }
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    *flag = 0;
    watchdogFlag_080bbd90 = flag;
    signal(SIGALRM, func_0804ac30);
    alarm(seconds);
    return 1;
}

int clearWatchdog()
{
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    watchdogFlag_080bbd90 = 0;
    return 1;
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
