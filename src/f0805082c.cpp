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
    return (t->unk_00 && t->unk_8c) || (!t->unk_00 && !t->unk_8c);
}

int func_0805085c(LocTag *t)
{
    return t->unk_8c;
}
