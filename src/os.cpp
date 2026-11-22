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
    reportInfo("OS_Init(Linux)\n");
    osTime_080bbd8c = time(0);
    return true;
}

static inline void resetWatchdogSignal()
{
    signal(SIGALRM, SIG_DFL);
}

bool setWatchdog(char *flag, unsigned int seconds)
{
    if (!flag) {
        reportError("setWatchdog - passed null ptr\n");
        return false;
    }
    alarm(0);
    resetWatchdogSignal();
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
        reportError("OS_mkTemp - passed null output ptr\n");
        return -1;
    }
    if (!name) {
        reportError("OS_mkTemp - passed null input base name\n");
        *pathOut = 0;
        return -1;
    }
    if (strlen(name) > 1023) {
        reportError("os_mkTemp : an unlikely error has occured\n");
        return -1;
    }
    sprintf(buf, "/tmp/%s.XXXXXX", name);
    fd = mkstemp(buf);
    if (fd < 0) {
        reportError("failed to make temp in file\n");
        *pathOut = 0;
        return -1;
    }
    mstrcpy(pathOut, buf);
    return fd;
}

int OS_End()
{
    reportInfo("OS_End(Linux)\n");
    return 1;
}

void func_0804ac30(int sig)
{
    if (watchdogFlag_080bbd90)
        *watchdogFlag_080bbd90 = 1;
}
