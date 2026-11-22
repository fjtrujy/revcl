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

bool suppressErrorReports = false;
int errorOccurred;
bool quietDiagnostics;
bool unhandledOptionQSeen;
bool unhandledOptionPSeen;
char *currentOutputFileName = 0;


void suppressFurtherErrors()
{
    suppressErrorReports = 1;
}

int reportMessage(const char *fmt, ...)
{
    if (quietDiagnostics)
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
    if (quietDiagnostics)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO: ");
    return vfprintf(stderr, fmt, ap);
}

int reportVerboseInfo(const char *fmt, ...)
{
    if (quietDiagnostics)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "INFO2: ");
    return vfprintf(stderr, fmt, ap);
}

int reportError(const char *fmt, ...)
{
    if (suppressErrorReports)
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
    suppressErrorReports = 1;
    fprintf(stderr, "USER: ");
    return vfprintf(stderr, fmt, ap);
}

int formatIntoBuffer(char *outputBuffer, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    return vsprintf(outputBuffer, fmt, ap);
}

int parseInteger(const char *s)
{
    return atoi(s);
}

void exitAfterCleanup(int status)
{
    if (status) {
        if (currentOutputFileName)
            printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", currentOutputFileName);
        else
            puts("ERROR: something went wrong, vcl's outta here.");
    }
    shutDownOperatingSystem();
    exit(status);
}

void exitWithError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (!suppressErrorReports) {
        errorOccurred = 1;
        fprintf(stderr, "ERROR: ");
        vfprintf(stderr, fmt, ap);
    }
    if (currentOutputFileName)
        printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", currentOutputFileName);
    else
        puts("ERROR: something went wrong, vcl's outta here.");
    shutDownOperatingSystem();
    exit(-1);
}
