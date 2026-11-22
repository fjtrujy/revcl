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
        reportError("UnlinkFromToDo - passed a null pointer\n");
        return false;
    }
    if (!blk->pendingInstructions.count) {
        reportError("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (n->pendingLink.nextInstruction)
        n->pendingLink.nextInstruction->pendingLink.previousInstruction = n->pendingLink.previousInstruction;
    else if (n == blk->pendingInstructions.tail)
        blk->pendingInstructions.tail = n->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (n->pendingLink.previousInstruction)
        n->pendingLink.previousInstruction->pendingLink.nextInstruction = n->pendingLink.nextInstruction;
    else if (n == blk->pendingInstructions.head)
        blk->pendingInstructions.head = n->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    blk->pendingInstructions.count--;
    n->pendingLink.previousInstruction = 0;
    n->pendingLink.nextInstruction = 0;
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n)
{
    if (!n) {
        reportError("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!blk->completedInstructions.count) {
        reportError("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (n->pendingLink.nextInstruction)
        n->pendingLink.nextInstruction->pendingLink.previousInstruction = n->pendingLink.previousInstruction;
    else if (n == blk->completedInstructions.tail)
        blk->completedInstructions.tail = n->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (n->pendingLink.previousInstruction)
        n->pendingLink.previousInstruction->pendingLink.nextInstruction = n->pendingLink.nextInstruction;
    else if (n == blk->completedInstructions.head)
        blk->completedInstructions.head = n->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    blk->completedInstructions.count--;
    n->pendingLink.previousInstruction = 0;
    n->pendingLink.nextInstruction = 0;
    return true;
}

bool UnlinkFromLinear(Block *blk, Instr *n)
{
    if (!n) {
        reportError("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!blk->linearInstructions.count) {
        reportError("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (n->linearLink.nextInstruction)
        n->linearLink.nextInstruction->linearLink.previousInstruction = n->linearLink.previousInstruction;
    else if (n == blk->linearInstructions.tail)
        blk->linearInstructions.tail = n->linearLink.previousInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (n->linearLink.previousInstruction)
        n->linearLink.previousInstruction->linearLink.nextInstruction = n->linearLink.nextInstruction;
    else if (n == blk->linearInstructions.head)
        blk->linearInstructions.head = n->linearLink.nextInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    blk->linearInstructions.count--;
    n->linearLink.previousInstruction = 0;
    n->linearLink.nextInstruction = 0;
    return true;
}

bool AddToDo_Head(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (n->pendingLink.previousInstruction || n->pendingLink.nextInstruction) {
        reportError("AddToDo_Head - node already appears linked\n");
        return false;
    }
    if (blk->pendingInstructions.head) {
        if (!blk->pendingInstructions.tail) {
            reportError("AddToDo_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->pendingInstructions.count) {
            reportError("AddToDo_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->pendingLink.nextInstruction = blk->pendingInstructions.head;
        blk->pendingInstructions.head->pendingLink.previousInstruction = n;
        blk->pendingInstructions.head = n;
    } else {
        if (blk->pendingInstructions.count) {
            reportError("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        blk->pendingInstructions.tail = n;
        blk->pendingInstructions.head = n;
    }
    blk->pendingInstructions.count++;
    return true;
}

bool AddDone_Head(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddDone_Head - passed a null pointer\n");
        return false;
    }
    if (n->pendingLink.previousInstruction || n->pendingLink.nextInstruction) {
        reportError("AddDone_Head - node already appears linked\n");
        return false;
    }
    if (blk->completedInstructions.head) {
        if (!blk->completedInstructions.tail) {
            reportError("AddDone_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->completedInstructions.count) {
            reportError("AddDone_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->pendingLink.nextInstruction = blk->completedInstructions.head;
        blk->completedInstructions.head->pendingLink.previousInstruction = n;
        blk->completedInstructions.head = n;
    } else {
        if (blk->completedInstructions.count) {
            reportError("AddDone_Headno list head but count not zero\n");
            return false;
        }
        blk->completedInstructions.tail = n;
        blk->completedInstructions.head = n;
    }
    blk->completedInstructions.count++;
    return true;
}

bool AddLinear_Head(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddLinear_Head - passed a null pointer\n");
        return false;
    }
    if (n->linearLink.previousInstruction || n->linearLink.nextInstruction) {
        reportError("AddLinear_Head - node already appears linked\n");
        return false;
    }
    if (blk->linearInstructions.head) {
        if (!blk->linearInstructions.tail) {
            reportError("AddLinear_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->linearInstructions.count) {
            reportError("AddLinear_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->linearLink.nextInstruction = blk->linearInstructions.head;
        blk->linearInstructions.head->linearLink.previousInstruction = n;
        blk->linearInstructions.head = n;
    } else {
        if (blk->linearInstructions.count) {
            reportError("AddLinear_Headno list head but count not zero\n");
            return false;
        }
        blk->linearInstructions.tail = n;
        blk->linearInstructions.head = n;
    }
    blk->linearInstructions.count++;
    return true;
}

bool AddToDo_Tail(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddToDo_Tail - passed a null pointer\n");
        return false;
    }
    if (n->pendingLink.previousInstruction || n->pendingLink.nextInstruction) {
        reportError("AddToDo_Tail - node already appears linked\n");
        return false;
    }
    if (blk->pendingInstructions.head) {
        if (!blk->pendingInstructions.tail) {
            reportError("AddToDo_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->pendingInstructions.count) {
            reportError("AddToDo_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->pendingLink.previousInstruction = blk->pendingInstructions.tail;
        blk->pendingInstructions.tail->pendingLink.nextInstruction = n;
        blk->pendingInstructions.tail = n;
    } else {
        if (blk->pendingInstructions.count) {
            reportError("AddToDo_Tailno list head but count not zero\n");
            return false;
        }
        blk->pendingInstructions.tail = n;
        blk->pendingInstructions.head = n;
    }
    blk->pendingInstructions.count++;
    return true;
}

bool AddDone_Tail(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddDone_Tail - passed a null pointer\n");
        return false;
    }
    if (n->pendingLink.previousInstruction || n->pendingLink.nextInstruction) {
        reportError("AddDone_Tail - node already appears linked\n");
        return false;
    }
    if (blk->completedInstructions.head) {
        if (!blk->completedInstructions.tail) {
            reportError("AddDone_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->completedInstructions.count) {
            reportError("AddDone_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->pendingLink.previousInstruction = blk->completedInstructions.tail;
        blk->completedInstructions.tail->pendingLink.nextInstruction = n;
        blk->completedInstructions.tail = n;
    } else {
        if (blk->completedInstructions.count) {
            reportError("AddDone_Tailno list head but count not zero\n");
            return false;
        }
        blk->completedInstructions.tail = n;
        blk->completedInstructions.head = n;
    }
    blk->completedInstructions.count++;
    return true;
}

bool AddLinear_Tail(Block *blk, Instr *n)
{
    if (!n) {
        reportError("AddLinear_Tail - passed a null pointer\n");
        return false;
    }
    if (n->linearLink.previousInstruction || n->linearLink.nextInstruction) {
        reportError("AddLinear_Tail - node already appears linked\n");
        return false;
    }
    if (blk->linearInstructions.head) {
        if (!blk->linearInstructions.tail) {
            reportError("AddLinear_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!blk->linearInstructions.count) {
            reportError("AddLinear_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        n->linearLink.previousInstruction = blk->linearInstructions.tail;
        blk->linearInstructions.tail->linearLink.nextInstruction = n;
        blk->linearInstructions.tail = n;
    } else {
        if (blk->linearInstructions.count) {
            reportError("AddLinear_Tailno list head but count not zero\n");
            return false;
        }
        blk->linearInstructions.tail = n;
        blk->linearInstructions.head = n;
    }
    blk->linearInstructions.count++;
    return true;
}

bool AddLinear_After(Block *blk, Instr *after, Instr *n)
{
    if (!n) {
        reportError("AddLinear_After - passed a null in_a pointer\n");
        return false;
    }
    if (n->linearLink.previousInstruction || n->linearLink.nextInstruction) {
        reportError("AddLinear_After - node already appears linked\n");
        return false;
    }
    if (after) {
        n->linearLink.nextInstruction = after->linearLink.nextInstruction;
        n->linearLink.previousInstruction = after;
        after->linearLink.nextInstruction = n;
    } else {
        n->linearLink.nextInstruction = blk->linearInstructions.head;
        blk->linearInstructions.head = n;
    }
    if (n->linearLink.nextInstruction)
        n->linearLink.nextInstruction->linearLink.previousInstruction = n;
    else
        blk->linearInstructions.tail = n;
    blk->linearInstructions.count++;
    return true;
}

bool equalAsciiCaseInsensitive(char a, char b)
{
    if (a >= 'a' && a <= 'z')
        a -= 32;
    if (b >= 'a' && b <= 'z')
        b -= 32;
    return a == b;
}

int countSetBits(unsigned int x)
{
    int n = 0;
    while (x) {
        n += x & 1;
        x >>= 1;
    }
    return n;
}

const char *tokenAllowedCharacters =
    "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?";
const char *identifierAllowedCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@";

bool isTokenAlphabetCharacter(char c)
{
    const char *p = tokenAllowedCharacters;
    char ch;
    while ((ch = *p) != 0 && ch != c)
        p++;
    return ch != 0;
}

bool isIdentifierAlphabetCharacter(char c)
{
    const char *p = identifierAllowedCharacters;
    char ch;
    while ((ch = *p) != 0 && ch != c)
        p++;
    return ch != 0;
}

bool asciiCaseEqual(const char *left, const char *right)
{
    if (!left || !right)
        return false;
    while (*left && *right) {
        char ca = *left;
        char cb = *right;
        if (ca >= 'a' && ca <= 'z')
            ca -= 32;
        if (cb >= 'a' && cb <= 'z')
            cb -= 32;
        if (cb != ca)
            return false;
        left++;
        right++;
    }
    if (*left || *right)
        return false;
    return true;
}

bool equalExactStrings(const char *a, const char *b)
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

void removeLastDotSuffix(char *s)
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

void removeTrailingComponentSuffix(char *s)
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
        reportError("dupString: passed null ptr\n");
        return 0;
    }
    const char *e = s;
    while (*e)
        e++;
    char *d = new char[e - s + 1];
    if (!d) {
        reportError("dupString: new failed to get ram\n");
        reportError("aborting.. sorry\n");
        exitAfterCleanup(-1);
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
        reportError("dupLabelString: null ptr input\n");
        return 0;
    }
    const char *e = s;
    while (*e)
        e++;
    char *d = new char[e - s + 1];
    if (!d) {
        reportError("dupLabelString: failed to get ram input\n");
        return 0;
    }
    char *q = d;
    while (*s && *s != ':')
        *q++ = *s++;
    *q = 0;
    return d;
}

void clearRawBytes(char *p, int n)
{
    if (!p) {
        reportError("asked to zero memory location 0\n");
        exitAfterCleanup(-1);
    }
    if (n < 0) {
        reportError("asked to zero -ve size\n");
        exitAfterCleanup(-1);
    }
    char *q = p;
    for (; n; n--)
        *q++ = 0;
}

int mstrlen(const char *s)
{
    if (!s) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    const char *e = s;
    while (*e)
        e++;
    return e - s;
}

char *mstrcpy(char *dst, const char *src)
{
    if (!dst) {
        reportError("mstrcpy passed null dst ptr\n");
        exitAfterCleanup(-1);
    }
    if (!src) {
        reportError("mstrcpy passed null src ptr\n");
        exitAfterCleanup(-1);
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
        reportError("mprefix_del - not str\n");
        return 0;
    }
    const char *e = a;
    while (*e)
        e++;
    int la = e - a;
    if (!b) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    e = b;
    while (*e)
        e++;
    char *r = new char[la + (e - b) + 2];
    const char *s = a;
    char *p = r;
    if (!r) {
        reportError("failed to get a string\n");
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
