// OS abstraction layer (Linux): logging, temp files, watchdog.
#ifndef OS_H
#define OS_H

struct Tool {
    const char *name;
    const char *cmd;
};

extern Tool tools_080b301c[3];

bool OS_Init();
bool setWatchdog(char *flag, unsigned int seconds);
bool clearWatchdog();

int func_0804ab6c(const char *name, char *pathOut);
int OS_End();
void func_0804ac44();
char *func_0804aef0(const char *path, int *len, bool flag);
char *func_0804aff0(int fd, int *len, bool nulTerm);
int reportMessage(const char *fmt, ...);
int reportWarning(const char *fmt, ...);
int reportInfo(const char *fmt, ...);
int reportVerboseInfo(const char *fmt, ...);
int reportError(const char *fmt, ...);
int reportUserMessage(const char *fmt, ...);
int formatIntoBuffer(char *buf, const char *fmt, ...);
const char *func_0806c6a0(int dev);  // device name
int parseInteger(const char *s);
void exitAfterCleanup(int status);
void exitWithError(const char *fmt, ...);
void abortAfterError();
bool asciiCaseEqual(const char *a, const char *b);

char *mstrcpy(char *dst, const char *src);

#endif
