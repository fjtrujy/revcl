// OS abstraction layer (Linux): logging, temp files, watchdog.
#ifndef OS_H
#define OS_H

struct Tool {
    const char *name;
    const char *cmd;
};

extern Tool tools_080b301c[3];

int OS_Init();
int setWatchdog(char *flag, unsigned int seconds);
int clearWatchdog();

int func_0804ab6c(const char *name, char *pathOut);
int OS_End();
void func_0804ac44();
char *func_0804aef0(const char *path, int *len, bool flag);
int func_0804ac50(const char *fmt, ...);
int func_0804ac7c(const char *fmt, ...);
int func_0804acb0(const char *fmt, ...);
int func_0804acf4(const char *fmt, ...);
int func_0804ad38(const char *fmt, ...);

char *mstrcpy(char *dst, const char *src);

#endif
