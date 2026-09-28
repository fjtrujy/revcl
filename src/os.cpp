// OS layer for Linux (CIE group 1, after preprocess.cpp).

#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "os.h"

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
