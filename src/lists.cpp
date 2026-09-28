// Instruction list helpers: ToDo / Done / Linear lists (CIE group 1, after mList.cpp).

#include <stdio.h>
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
    if (!blk->unk_9c) {
        func_0804ad38("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24)
        n->unk_24->unk_28 = n->unk_28;
    else if (n == blk->unk_98)
        blk->unk_98 = n->unk_28;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (n->unk_28)
        n->unk_28->unk_24 = n->unk_24;
    else if (n == blk->unk_94)
        blk->unk_94 = n->unk_24;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    blk->unk_9c--;
    n->unk_28 = 0;
    n->unk_24 = 0;
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_a8) {
        func_0804ad38("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24)
        n->unk_24->unk_28 = n->unk_28;
    else if (n == blk->unk_a4)
        blk->unk_a4 = n->unk_28;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (n->unk_28)
        n->unk_28->unk_24 = n->unk_24;
    else if (n == blk->unk_a0)
        blk->unk_a0 = n->unk_24;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    blk->unk_a8--;
    n->unk_28 = 0;
    n->unk_24 = 0;
    return true;
}

bool UnlinkFromLinear(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_90) {
        func_0804ad38("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_1c)
        n->unk_1c->unk_20 = n->unk_20;
    else if (n == blk->unk_8c)
        blk->unk_8c = n->unk_20;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (n->unk_20)
        n->unk_20->unk_1c = n->unk_1c;
    else if (n == blk->unk_88)
        blk->unk_88 = n->unk_1c;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    blk->unk_90--;
    n->unk_20 = 0;
    n->unk_1c = 0;
    return true;
}

bool AddToDo_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_28 || n->unk_24) {
        func_0804ad38("AddToDo_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_94) {
        if (!blk->unk_98) {
            func_0804ad38("AddToDo_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_9c) {
            func_0804ad38("AddToDo_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24 = blk->unk_94;
        blk->unk_94->unk_28 = n;
        blk->unk_94 = n;
    } else {
        if (blk->unk_9c) {
            func_0804ad38("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_98 = n;
        blk->unk_94 = n;
    }
    blk->unk_9c++;
    return true;
}

bool AddDone_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddDone_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_28 || n->unk_24) {
        func_0804ad38("AddDone_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_a0) {
        if (!blk->unk_a4) {
            func_0804ad38("AddDone_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_a8) {
            func_0804ad38("AddDone_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24 = blk->unk_a0;
        blk->unk_a0->unk_28 = n;
        blk->unk_a0 = n;
    } else {
        if (blk->unk_a8) {
            func_0804ad38("AddDone_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_a4 = n;
        blk->unk_a0 = n;
    }
    blk->unk_a8++;
    return true;
}

bool AddLinear_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_20 || n->unk_1c) {
        func_0804ad38("AddLinear_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_88) {
        if (!blk->unk_8c) {
            func_0804ad38("AddLinear_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_90) {
            func_0804ad38("AddLinear_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_1c = blk->unk_88;
        blk->unk_88->unk_20 = n;
        blk->unk_88 = n;
    } else {
        if (blk->unk_90) {
            func_0804ad38("AddLinear_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_8c = n;
        blk->unk_88 = n;
    }
    blk->unk_90++;
    return true;
}

bool AddToDo_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddToDo_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_28 || n->unk_24) {
        func_0804ad38("AddToDo_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_94) {
        if (!blk->unk_98) {
            func_0804ad38("AddToDo_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_9c) {
            func_0804ad38("AddToDo_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_28 = blk->unk_98;
        blk->unk_98->unk_24 = n;
        blk->unk_98 = n;
    } else {
        if (blk->unk_9c) {
            func_0804ad38("AddToDo_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_98 = n;
        blk->unk_94 = n;
    }
    blk->unk_9c++;
    return true;
}

bool AddDone_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddDone_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_28 || n->unk_24) {
        func_0804ad38("AddDone_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_a0) {
        if (!blk->unk_a4) {
            func_0804ad38("AddDone_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_a8) {
            func_0804ad38("AddDone_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_28 = blk->unk_a4;
        blk->unk_a4->unk_24 = n;
        blk->unk_a4 = n;
    } else {
        if (blk->unk_a8) {
            func_0804ad38("AddDone_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_a4 = n;
        blk->unk_a0 = n;
    }
    blk->unk_a8++;
    return true;
}

bool AddLinear_Tail(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_Tail - passed a null pointer\n");
        return false;
    }
    if (n->unk_20 || n->unk_1c) {
        func_0804ad38("AddLinear_Tail - node already appears linked\n");
        return false;
    }
    if (blk->unk_88) {
        if (!blk->unk_8c) {
            func_0804ad38("AddLinear_Taillist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_90) {
            func_0804ad38("AddLinear_Taillist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_20 = blk->unk_8c;
        blk->unk_8c->unk_1c = n;
        blk->unk_8c = n;
    } else {
        if (blk->unk_90) {
            func_0804ad38("AddLinear_Tailno list head but count not zero\n");
            return false;
        }
        blk->unk_8c = n;
        blk->unk_88 = n;
    }
    blk->unk_90++;
    return true;
}

bool AddLinear_After(Block *blk, Instr *after, Instr *n)
{
    if (!n) {
        func_0804ad38("AddLinear_After - passed a null in_a pointer\n");
        return false;
    }
    if (n->unk_20 || n->unk_1c) {
        func_0804ad38("AddLinear_After - node already appears linked\n");
        return false;
    }
    if (after) {
        n->unk_1c = after->unk_1c;
        n->unk_20 = after;
        after->unk_1c = n;
    } else {
        n->unk_1c = blk->unk_88;
        blk->unk_88 = n;
    }
    if (n->unk_1c)
        n->unk_1c->unk_20 = n;
    else
        blk->unk_8c = n;
    blk->unk_90++;
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
    if (!s || !*s)
        return;
    int i = 0;
    while (s[i])
        i++;
    while (--i != 0 && (s[i] == 'x' || s[i] == 'X' || s[i] == 'y' || s[i] == 'Y' ||
                        s[i] == 'z' || s[i] == 'Z' || s[i] == 'w' || s[i] == 'W'))
        ;
    s[i + 1] = 0;
    if (s[i] == '.')
        s[i] = 0;
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

mValue::mValue()
{
    str = 0;
    type = NONE;
}

mValue::~mValue()
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
}

void mValue::clear()
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
}

int func_0804c4e8(int a, int b)
{
    if (b > a)
        a = b;
    return a;
}

int func_0804c4f8(int a, int b)
{
    int t = a;
    if (b > a)
        t = b;
    if (t == mValue::FLOAT)
        return mValue::NONE;
    return t;
}

bool mValue::getFloat(float *out)
{
    switch (type) {
    case UINT: *out = (float)u; break;
    case INT: *out = (float)i; break;
    case FLOAT: *out = f; break;
    default: return false;
    }
    return true;
}

bool mValue::getUInt(unsigned int *out)
{
    switch (type) {
    case UINT: *out = u; break;
    case INT: *out = i; break;
    case FLOAT: *out = (unsigned int)f; break;
    default: return false;
    }
    return true;
}

bool mValue::getInt(int *out)
{
    switch (type) {
    case UINT: *out = u; break;
    case INT: *out = i; break;
    case FLOAT: *out = (int)f; break;
    default: return false;
    }
    return true;
}

bool mValue::set(mValue *src)
{
    if (!src) {
        func_0804ad38("xpVal::set() - source is null ptr\n");
        return false;
    }
    if (src->type < UINT || src->type > STRING) {
        func_0804ad38("xpVal::set() - source is invalid\n");
        return false;
    }
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = src->type;
    if (type == STRING) {
        str = dupString(src->str);
        if (!str) {
            func_0804ad38("failed to dup literal\n");
            if (type == STRING) {
                if (str)
                    delete[] str;
                str = 0;
            }
            type = NONE;
            return false;
        }
    } else
        u = src->u;
    return true;
}

bool mValue::setInt(int v)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    i = v;
    type = INT;
    return true;
}

bool mValue::setString(const char *s)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = STRING;
    str = dupString(s);
    if (!str) {
        func_0804ad38("failed to dup string for set str\n");
        type = NONE;
        return false;
    }
    return true;
}

bool mValue::setString(const char *s, int n)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    if (!s)
        return false;
    type = STRING;
    str = new char[n + 1];
    if (!str) {
        func_0804ad38("failed to new chars for set str\n");
        type = NONE;
        return false;
    }
    const char *p = s;
    char *d = str;
    while (n && *p) {
        *d++ = *p++;
        n--;
    }
    *d = 0;
    return true;
}

bool mValue::setUInt(unsigned int v)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    u = v;
    type = UINT;
    return true;
}

bool mValue::setFloat(float v)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = FLOAT;
    f = v;
    return true;
}

bool mValue::isValid()
{
    return type >= UINT && type <= STRING;
}

bool mValue::isNumeric()
{
    return type >= UINT && type <= FLOAT;
}

bool mValue::isInt()
{
    return type >= UINT && type <= INT;
}

bool mValue::isFloat()
{
    return type == FLOAT;
}

bool mValue::negate(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = -u; break;
    case INT: i = -i; break;
    case FLOAT: f = -f; break;
    }
    return true;
}

bool mValue::abs(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: break;
    case INT: if (i < 0) i = -i; break;
    case FLOAT: if (f < 0) f = -f; break;
    }
    return true;
}

bool mValue::logicalNot(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = !u; break;
    case INT: i = !i; break;
    case FLOAT: return false;
    }
    return true;
}

bool mValue::bitNot(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = ~u; break;
    case INT: i = ~i; break;
    case FLOAT: return false;
    }
    return true;
}

const char *func_0804ca58(int type)
{
    switch (type) {
    case 3: return "EXP_DATATYPE_FLOAT";
    case 0: return "EXP_DATATYPE_NULL";
    case 1: return "EXP_DATATYPE_UINT";
    case 2: return "EXP_DATATYPE_SINT";
    case 4: return "EXP_DATATTYPE_LITERAL";
    }
    return "EXP_DATATYPE_UNKNOWN";
}

static inline bool getUIntInl(mValue *v, unsigned int *out)
{
    switch (v->type) {
    case mValue::UINT: *out = v->u; break;
    case mValue::INT: *out = v->i; break;
    case mValue::FLOAT: *out = (unsigned int)v->f; break;
    default: return false;
    }
    return true;
}

static inline bool getIntInl(mValue *v, int *out)
{
    switch (v->type) {
    case mValue::UINT: *out = v->u; break;
    case mValue::INT: *out = v->i; break;
    case mValue::FLOAT: *out = (int)v->f; break;
    default: return false;
    }
    return true;
}

static inline bool getFloatInl(mValue *v, float *out)
{
    switch (v->type) {
    case mValue::UINT: *out = (float)v->u; break;
    case mValue::INT: *out = (float)v->i; break;
    case mValue::FLOAT: *out = v->f; break;
    default: return false;
    }
    return true;
}

bool func_0804ca94(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    if (!a)
        return false;
    if (!b)
        return false;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        return false;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        return false;
    int t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch (t) {
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua + ub;
        break;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            func_0804ad38("failed to get args\n");
            return false;
        }
        dst->i = ia + ib;
        break;
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa + fb;
            break;
        }
        func_0804ad38("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804cd68(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    if (!a)
        return false;
    if (!b)
        return false;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        return false;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        return false;
    int t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch (t) {
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua - ub;
        break;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            func_0804ad38("failed to get args\n");
            return false;
        }
        dst->i = ia - ib;
        break;
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa - fb;
            break;
        }
        func_0804ad38("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804d040(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    if (!a)
        return false;
    if (!b)
        return false;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        return false;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        return false;
    int t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua * ub;
        break;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            func_0804ad38("failed to get args\n");
            return false;
        }
        dst->i = ia * ib;
        break;
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa * fb;
            break;
        }
        func_0804ad38("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804d314(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b)
        return false;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        return false;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        return false;
    int t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua / ub;
        break;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            func_0804ad38("failed to get args\n");
            return false;
        }
        dst->i = ia / ib;
        break;
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa / fb;
            break;
        }
        func_0804ad38("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

static inline bool isNumType(unsigned int t)
{
    return t >= mValue::UINT && t <= mValue::FLOAT;
}

static inline int promoteIntType(int a, int b)
{
    int t = a;
    if (b > a)
        t = b;
    if (t == mValue::FLOAT)
        return mValue::NONE;
    return t;
}

bool func_0804d5ec(mValue *dst, mValue *a, mValue *b)
{
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b) {
        printf("xpVal::mod null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::mod invalid arg\n");
        return false;
    }
    int t = promoteIntType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::mod type promote failed\n");
        return false;
    }
    switch (t) {
    case mValue::UINT:
        if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
            dst->u = ua % ub;
            break;
        }
        printf("failed to get args\n");
        return false;
    case mValue::INT:
        if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
            dst->i = ia % ib;
            break;
        }
        printf("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804d7d4(mValue *dst, mValue *a, mValue *b)
{
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b) {
        printf("xpVal::shfl null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::shfl invalid arg\n");
        return false;
    }
    int t = promoteIntType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::shfl type promote failed\n");
        return false;
    }
    switch (t) {
    case mValue::UINT:
        if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
            dst->u = ua << ub;
            break;
        }
        printf("failed to get args\n");
        return false;
    case mValue::INT:
        if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
            dst->i = ia << ib;
            break;
        }
        printf("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804d9c0(mValue *dst, mValue *a, mValue *b)
{
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b) {
        printf("xpVal::shfr null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::shfr invalid arg\n");
        return false;
    }
    int t = promoteIntType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::shfr type promote failed\n");
        return false;
    }
    switch (t) {
    case mValue::UINT:
        if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
            dst->u = ua >> ub;
            break;
        }
        printf("failed to get args\n");
        return false;
    case mValue::INT:
        if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
            dst->i = ia >> ib;
            break;
        }
        printf("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}

bool func_0804dbac(mValue *dst, mValue *a, mValue *b)
{
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b) {
        printf("xpVal::logor null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::logor invalid arg\n");
        return false;
    }
    int t = promoteIntType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::logor type promote failed\n");
        return false;
    }
    switch (t) {
    case mValue::UINT:
        if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
            dst->u = ua || ub;
            break;
        }
        printf("failed to get args\n");
        return false;
    case mValue::INT:
        if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
            dst->i = ia || ib;
            break;
        }
        printf("failed to get args\n");
        return false;
    default:
        printf("bad type to expr\n");
        return false;
    }
    return true;
}
