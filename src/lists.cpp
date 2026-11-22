// Instruction list helpers: ToDo / Done / Linear lists (CIE group 1, after mList.cpp).

#include <stdio.h>
#include <stdlib.h>
#include "Block.h"
#include "Instr.h"
#include "mValue.h"
#include "os.h"

bool UnlinkFromToDo(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromToDo - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_94.unk_08) {
        func_0804ad38("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24.unk_00)
        n->unk_24.unk_00->unk_24.unk_04 = n->unk_24.unk_04;
    else if (n == blk->unk_94.unk_04)
        blk->unk_94.unk_04 = n->unk_24.unk_04;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (n->unk_24.unk_04)
        n->unk_24.unk_04->unk_24.unk_00 = n->unk_24.unk_00;
    else if (n == blk->unk_94.unk_00)
        blk->unk_94.unk_00 = n->unk_24.unk_00;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    blk->unk_94.unk_08--;
    n->unk_24.unk_04 = 0;
    n->unk_24.unk_00 = 0;
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_a0.unk_08) {
        func_0804ad38("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24.unk_00)
        n->unk_24.unk_00->unk_24.unk_04 = n->unk_24.unk_04;
    else if (n == blk->unk_a0.unk_04)
        blk->unk_a0.unk_04 = n->unk_24.unk_04;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (n->unk_24.unk_04)
        n->unk_24.unk_04->unk_24.unk_00 = n->unk_24.unk_00;
    else if (n == blk->unk_a0.unk_00)
        blk->unk_a0.unk_00 = n->unk_24.unk_00;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    blk->unk_a0.unk_08--;
    n->unk_24.unk_04 = 0;
    n->unk_24.unk_00 = 0;
    return true;
}

bool UnlinkFromLinear(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_88.unk_08) {
        func_0804ad38("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_1c.unk_00)
        n->unk_1c.unk_00->unk_1c.unk_04 = n->unk_1c.unk_04;
    else if (n == blk->unk_88.unk_04)
        blk->unk_88.unk_04 = n->unk_1c.unk_04;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (n->unk_1c.unk_04)
        n->unk_1c.unk_04->unk_1c.unk_00 = n->unk_1c.unk_00;
    else if (n == blk->unk_88.unk_00)
        blk->unk_88.unk_00 = n->unk_1c.unk_00;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    blk->unk_88.unk_08--;
    n->unk_1c.unk_04 = 0;
    n->unk_1c.unk_00 = 0;
    return true;
}

bool AddToDo_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_24.unk_04 || n->unk_24.unk_00) {
        func_0804ad38("AddToDo_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_94.unk_00) {
        if (!blk->unk_94.unk_04) {
            func_0804ad38("AddToDo_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_94.unk_08) {
            func_0804ad38("AddToDo_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24.unk_00 = blk->unk_94.unk_00;
        blk->unk_94.unk_00->unk_24.unk_04 = n;
        blk->unk_94.unk_00 = n;
    } else {
        if (blk->unk_94.unk_08) {
            func_0804ad38("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_94.unk_04 = n;
        blk->unk_94.unk_00 = n;
    }
    blk->unk_94.unk_08++;
    return true;
}

bool AddDone_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddDone_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_24.unk_04 || n->unk_24.unk_00) {
        func_0804ad38("AddDone_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_a0.unk_00) {
        if (!blk->unk_a0.unk_04) {
            func_0804ad38("AddDone_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_a0.unk_08) {
            func_0804ad38("AddDone_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24.unk_00 = blk->unk_a0.unk_00;
        blk->unk_a0.unk_00->unk_24.unk_04 = n;
        blk->unk_a0.unk_00 = n;
    } else {
        if (blk->unk_a0.unk_08) {
            func_0804ad38("AddDone_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_a0.unk_04 = n;
        blk->unk_a0.unk_00 = n;
    }
    blk->unk_a0.unk_08++;
    return true;
}

bool AddLinear_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_1c.unk_04 || n->unk_1c.unk_00) {
        func_0804ad38("AddLinear_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_88.unk_00) {
        if (!blk->unk_88.unk_04) {
            func_0804ad38("AddLinear_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_88.unk_08) {
            func_0804ad38("AddLinear_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_1c.unk_00 = blk->unk_88.unk_00;
        blk->unk_88.unk_00->unk_1c.unk_04 = n;
        blk->unk_88.unk_00 = n;
    } else {
        if (blk->unk_88.unk_08) {
            func_0804ad38("AddLinear_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_88.unk_04 = n;
        blk->unk_88.unk_00 = n;
    }
    blk->unk_88.unk_08++;
    return true;
}

bool AddToDo_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddToDo_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_24.unk_04 || n->unk_24.unk_00) {
        func_0804ad38("AddToDo_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_94.unk_00) {
        if (!blk->unk_94.unk_04) {
            func_0804ad38("AddToDo_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_94.unk_08) {
            func_0804ad38("AddToDo_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24.unk_04 = blk->unk_94.unk_04;
        blk->unk_94.unk_04->unk_24.unk_00 = n;
        blk->unk_94.unk_04 = n;
    } else {
        if (blk->unk_94.unk_08) {
            func_0804ad38("AddToDo_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_94.unk_04 = n;
        blk->unk_94.unk_00 = n;
    }
    blk->unk_94.unk_08++;
    return true;
}

bool AddDone_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddDone_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_24.unk_04 || n->unk_24.unk_00) {
        func_0804ad38("AddDone_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_a0.unk_00) {
        if (!blk->unk_a0.unk_04) {
            func_0804ad38("AddDone_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_a0.unk_08) {
            func_0804ad38("AddDone_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24.unk_04 = blk->unk_a0.unk_04;
        blk->unk_a0.unk_04->unk_24.unk_00 = n;
        blk->unk_a0.unk_04 = n;
    } else {
        if (blk->unk_a0.unk_08) {
            func_0804ad38("AddDone_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_a0.unk_04 = n;
        blk->unk_a0.unk_00 = n;
    }
    blk->unk_a0.unk_08++;
    return true;
}

bool AddLinear_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_1c.unk_04 || n->unk_1c.unk_00) {
        func_0804ad38("AddLinear_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_88.unk_00) {
        if (!blk->unk_88.unk_04) {
            func_0804ad38("AddLinear_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_88.unk_08) {
            func_0804ad38("AddLinear_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_1c.unk_04 = blk->unk_88.unk_04;
        blk->unk_88.unk_04->unk_1c.unk_00 = n;
        blk->unk_88.unk_04 = n;
    } else {
        if (blk->unk_88.unk_08) {
            func_0804ad38("AddLinear_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_88.unk_04 = n;
        blk->unk_88.unk_00 = n;
    }
    blk->unk_88.unk_08++;
    return true;
}

bool AddLinear_After(Block *blk, Instr *after, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_After - passed a null in_a pointer\n");
        return false;
    }
    if (n->unk_1c.unk_04 || n->unk_1c.unk_00) {
        func_0804ad38("AddLinear_After - node already appears linked\n");
        return false;
    }
    if (after) {
        n->unk_1c.unk_00 = after->unk_1c.unk_00;
        n->unk_1c.unk_04 = after;
        after->unk_1c.unk_00 = n;
    } else {
        n->unk_1c.unk_00 = blk->unk_88.unk_00;
        blk->unk_88.unk_00 = n;
    }
    if (n->unk_1c.unk_00)
        n->unk_1c.unk_00->unk_1c.unk_04 = n;
    else
        blk->unk_88.unk_04 = n;
    blk->unk_88.unk_08++;
    return true;
}

bool func_0804bf28(char a, char b)
{
    if (a >= 'a' && a <= 'z')
        a -= 32;
    if (b >= 'a' && b <= 'z')
        b -= 32;
    return a == b;
}

int func_0804bf50(unsigned int x)
{
    int n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }
    return n;
}

const char *tokenChars_080b3080 =
    "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?";
const char *identChars_080b3084 =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@";

bool func_0804bf6c(char c)
{
    const char *p = tokenChars_080b3080;
    char ch;
    while ((ch = *p) != 0 && ch != c)
        p++;
    return ch != 0;
}

bool func_0804bf9c(char c)
{
    const char *p = identChars_080b3084;
    char ch;
    while ((ch = *p) != 0 && ch != c)
        p++;
    return ch != 0;
}

bool func_0804bfcc(const char *a, const char *b)
{
    if (!a || !b)
        return false;
    while (*a && *b) {
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z')
            ca -= 32;
        if (cb >= 'a' && cb <= 'z')
            cb -= 32;
        if (cb != ca)
            return false;
        a++;
        b++;
    }
    if (*a || *b)
        return false;
    return true;
}

bool func_0804c03c(const char *a, const char *b)
{
    if (!a || !b)
        return false;
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    if (*a || *b)
        return false;
    return true;
}

void func_0804c08c(char *s)
{
    char *p = s;
    if (!s || !*s)
        return;
    while (*p)
        p++;
    p--;
    while (p != s && *p != '.')
        p--;
    if (*p == '.')
        *p = 0;
}

void func_0804c0c8(char *s)
{
    char *p = s;
    if (!s || !*s)
        return;
    while (*p)
        p++;
    for (--p; p != s && (*p == 'x' || *p == 'X' || *p == 'y' || *p == 'Y' ||
                        *p == 'z' || *p == 'Z' || *p == 'w' || *p == 'W'); --p)
        ;
    p[1] = 0;
    if (*p == '.')
        *p = 0;
}

char *dupString(const char *s)
{
    if (!s) {
        func_0804ad38("dupString: passed null ptr\n");
        return 0;
    }
    const char *e = s;
    while (*e)
        e++;
    char *d = new char[e - s + 1];
    if (!d) {
        func_0804ad38("dupString: new failed to get ram\n");
        func_0804ad38("aborting.. sorry\n");
        func_0804adf0(-1);
    }
    char *q = d;
    while (*s)
        *q++ = *s++;
    *q = 0;
    return d;
}

char *dupLabelString(const char *s)
{
    if (!s) {
        func_0804ad38("dupLabelString: null ptr input\n");
        return 0;
    }
    const char *e = s;
    while (*e)
        e++;
    char *d = new char[e - s + 1];
    if (!d) {
        func_0804ad38("dupLabelString: failed to get ram input\n");
        return 0;
    }
    char *q = d;
    while (*s && *s != ':')
        *q++ = *s++;
    *q = 0;
    return d;
}

void func_0804c220(char *p, int n)
{
    if (!p) {
        func_0804ad38("asked to zero memory location 0\n");
        func_0804adf0(-1);
    }
    if (n < 0) {
        func_0804ad38("asked to zero -ve size\n");
        func_0804adf0(-1);
    }
    char *q = p;
    for (; n; n--)
        *q++ = 0;
}

int mstrlen(const char *s)
{
    if (!s) {
        func_0804ad38("mstrlen passed null ptr\n");
        func_0804adf0(-1);
    }
    const char *e = s;
    while (*e)
        e++;
    return e - s;
}

char *mstrcpy(char *dst, const char *src)
{
    if (!dst) {
        func_0804ad38("mstrcpy passed null dst ptr\n");
        func_0804adf0(-1);
    }
    if (!src) {
        func_0804ad38("mstrcpy passed null src ptr\n");
        func_0804adf0(-1);
    }
    const char *s = src;
    char *d = dst;
    while (*s)
        *d++ = *s++;
    *d = 0;
    return dst;
}

char *mprefix_del(const char *a, const char *b)
{
    if (!a || !b) {
        func_0804ad38("mprefix_del - not str\n");
        return 0;
    }
    const char *e = a;
    while (*e)
        e++;
    int la = e - a;
    if (!b) {
        func_0804ad38("mstrlen passed null ptr\n");
        func_0804adf0(-1);
    }
    e = b;
    while (*e)
        e++;
    char *r = new char[la + (e - b) + 2];
    const char *s = a;
    char *p = r;
    if (!r) {
        func_0804ad38("failed to get a string\n");
        return 0;
    }
    while (*s)
        *p++ = *s++;
    *p++ = '_';
    s = b;
    while (*s)
        *p++ = *s++;
    *p = 0;
    return r;
}
