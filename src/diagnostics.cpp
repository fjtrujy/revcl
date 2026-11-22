// Linux logging and fatal-error helpers (CIE group 1).

#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "os.h"

bool flag_080b3038 = false;
int errorOccurred;
bool quiet_080bbd98;
bool g_080bbd99;
bool g_080bbd9a;
char *outName_080b303c = 0;


void func_0804ac44()
{
    flag_080b3038 = 1;
}

int reportMessage(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    return vfprintf(stderr, fmt, ap);
}

int reportWarning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "WARN: ");
    return vfprintf(stderr, fmt, ap);
}

int reportInfo(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO: ");
    return vfprintf(stderr, fmt, ap);
}

int reportVerboseInfo(const char *fmt, ...)
{
    if (quiet_080bbd98)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO2: ");
    return vfprintf(stderr, fmt, ap);
}

int reportError(const char *fmt, ...)
{
    if (flag_080b3038)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    errorOccurred = 1;
    fprintf(stderr, "ERROR: ");
    return vfprintf(stderr, fmt, ap);
}

int reportUserMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    flag_080b3038 = 1;
    fprintf(stderr, "USER: ");
    return vfprintf(stderr, fmt, ap);
}

int formatIntoBuffer(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    return vsprintf(buf, fmt, ap);
}

int parseInteger(const char *s)
{
    return atoi(s);
}

void exitAfterCleanup(int status)
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

void exitWithError(const char *fmt, ...)
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
