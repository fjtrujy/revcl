// Unknown-name list entry (name from "failed to new xun"), sizeof 8, no vptr.
#ifndef XUN_H
#define XUN_H

class xun {
public:
    xun *next;
    char *name;

    enum InitTag { INIT };
    xun();
    inline xun(InitTag);
    ~xun();
};

extern xun *g_080bcda4;
extern int g_080bcda8;

void clearUnknownSymbols();
bool recordUnknownSymbol(const char *s);
void visitUnknownSymbols();

#endif
