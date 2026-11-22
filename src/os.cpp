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

extern bool suppressErrorReports;
extern bool quietDiagnostics;
extern int errorOccurred;
extern char *currentOutputFileName;

int hostClockCounter;
char *watchdogTimeoutFlag;

void watchdogSignalHandler(int sig);

int nextHostClockTick()
{
    return ++hostClockCounter;
}

bool OS_Init()
{
    reportInfo("OS_Init(Linux)\n");
    hostClockCounter = time(0);
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
    watchdogTimeoutFlag = flag;
    signal(SIGALRM, watchdogSignalHandler);
    alarm(seconds);
    return true;
}

bool clearWatchdog()
{
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    watchdogTimeoutFlag = 0;
    return true;
}

int createTemporaryFile(const char *name, char *pathOut)
{
    char buf[1024];
    int temporaryDescriptor = -1;

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
    temporaryDescriptor = mkstemp(buf);
    if (temporaryDescriptor < 0) {
        reportError("failed to make temp in file\n");
        *pathOut = 0;
        return -1;
    }
    mstrcpy(pathOut, buf);
    return temporaryDescriptor;
}

int OS_End()
{
    reportInfo("OS_End(Linux)\n");
    return 1;
}

void watchdogSignalHandler(int sig)
{
    if (watchdogTimeoutFlag)
        *watchdogTimeoutFlag = 1;
}
