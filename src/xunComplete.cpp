// User-authorized complete-destructor emission and real lookup ownership.

#define XUN_NO_INTERFACE
#include "xun.h"
#include "xpNode.h"

xpOp *func_0804f6c0(int type)
{
    for (int i = 0; i != 20; i++)
        if (opTable_080b30a0[i].type == type)
            return &opTable_080b30a0[i];
    return 0;
}

// The original cleanup remains the strong implementation in ParserCtor.cpp.
void func_0804f6ec() __attribute__((weak));

void func_0804f6ec()
{
    xun *p = g_080bcda4;
    while (p) {
        xun *next = p->next;
        g_080bcda4 = next;
        delete p;
        p = next;
    }
    g_080bcda4 = 0;
    g_080bcda8 = 0;
}
