// Unknown-name list (CIE group 2, after xpNode.cpp).

#include "xun.h"
#include "os.h"
#include "xpNode.h"

bool func_0804bfcc(const char *a, const char *b);
char *dupString(const char *s);

xun *g_080bcda4;
int g_080bcda8;

xpOp *func_0804f6c0(int type)
{
    for (int i = 0; i != 20; i++)
        if (opTable_080b30a0[i].type == type)
            return &opTable_080b30a0[i];
    return 0;
}

void func_0804f6ec()
{
    while (g_080bcda4) {
        xun *p = g_080bcda4;
        g_080bcda4 = p->next;
        delete p;
    }
    g_080bcda4 = 0;
    g_080bcda8 = 0;
}

bool func_0804f75c(const char *s)
{
    if (!s) {
        func_0804ad38("passed a null string to add unknown\n");
        return false;
    }
    for (xun *p = g_080bcda4; p; p = p->next)
        if (func_0804bfcc(p->name, s))
            return true;
    xun *x = new xun;
    if (!x) {
        func_0804ad38("failed to new xun\n");
        return false;
    }
    x->name = dupString(s);
    if (!x->name) {
        func_0804ad38("failed to dup string for xun\n");
        delete x;
        return false;
    }
    g_080bcda8++;
    x->next = g_080bcda4;
    g_080bcda4 = x;
    return true;
}

void func_0804f844()
{
    for (xun *p = g_080bcda4; p; p = p->next)
        ;
}
