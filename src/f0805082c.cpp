// Source parser (CIE group 2, after xun.cpp).
// xpTree unknown-collection helpers live at the head of this unit (address order).

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


bool func_0805082c(LocTag *t)
{
    bool r = false;
    if (t->unk_00) {
        if (t->unk_8c)
            r = true;
    } else if (!t->unk_8c)
        r = true;
    return r;
}

int func_0805085c(LocTag *t)
{
    return t->unk_8c;
}
