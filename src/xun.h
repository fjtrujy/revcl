// Unknown-name list entry (name from "failed to new xun"), sizeof 8, no vptr.
#ifndef XUN_H
#define XUN_H

#pragma interface

class xun {
public:
    xun *next;
    char *name;

    xun() { next = 0; name = 0; }
    ~xun()
    {
        delete[] name;
        name = 0;
        next = 0;
    }
};

extern xun *g_080bcda4;
extern int g_080bcda8;

void func_0804f6ec();
bool func_0804f75c(const char *s);
void func_0804f844();

#endif
