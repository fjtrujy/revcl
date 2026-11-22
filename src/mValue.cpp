// mValue: typed value (int / uint / float / string) and string helpers.

#include <stdio.h>
#include <stdlib.h>
#include "Block.h"
#include "Instr.h"
#include "mValue.h"
#include "os.h"

bool g_080b3088 = false;
const char *g_080b308c =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@.[]";
const char *g_080b3090[3] = { "0123456789", "0123456789abcdef", "01" };

bool asciiCaseEqual(const char *a, const char *b);
char *dupString(const char *s);
int mstrlen(const char *s);

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

int maximumValueType(int a, int b)
{
    if (b > a)
        a = b;
    return a;
}

int promoteIntegralValueType(int a, int b)
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
        reportError("xpVal::set() - source is null ptr\n");
        return false;
    }
    if (src->type < UINT || src->type > STRING) {
        reportError("xpVal::set() - source is invalid\n");
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
            reportError("failed to dup literal\n");
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
        reportError("failed to dup string for set str\n");
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
        reportError("failed to new chars for set str\n");
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

bool addValues(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    int t;
    if (!a)
        goto fail;
    if (!b)
        goto fail;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        goto fail;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        goto fail;
    t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa + fb;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = ia + ib;
        return true;
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua + ub;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool subtractValues(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    int t;
    if (!a)
        goto fail;
    if (!b)
        goto fail;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        goto fail;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        goto fail;
    t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa - fb;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = ia - ib;
        return true;
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua - ub;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool multiplyValues(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    int t;
    if (!a)
        goto fail;
    if (!b)
        goto fail;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        goto fail;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        goto fail;
    t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa * fb;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = ia * ib;
        return true;
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua * ub;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool divideValues(mValue *dst, mValue *a, mValue *b)
{
    float fa, fb;
    int ia, ib;
    unsigned int ua, ub;
    int t;
    if (!a)
        goto fail;
    if (!b)
        goto fail;
    if (a->type < mValue::UINT || a->type > mValue::FLOAT)
        goto fail;
    if (b->type < mValue::UINT || b->type > mValue::FLOAT)
        goto fail;
    t = a->type;
    if ((int)b->type > t)
        t = b->type;
    dst->type = t;
    if (t == mValue::NONE)
        return false;
    switch ((unsigned)t) {
    case mValue::FLOAT:
        if (getFloatInl(a, &fa) && getFloatInl(b, &fb)) {
            dst->f = fa / fb;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(a, &ia) || !getIntInl(b, &ib)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = ia / ib;
        return true;
    case mValue::UINT:
        if (!getUIntInl(a, &ua) || !getUIntInl(b, &ub)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = ua / ub;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
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

bool moduloValues(mValue *dst, mValue *a, mValue *b)
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
    if (t != mValue::UINT) {
        if (t == mValue::INT) {
            if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
                dst->i = ia % ib;
                goto success;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
        dst->u = ua % ub;
        goto success;
    }
    printf("failed to get args\n");
    return false;
success:
    return true;
}

bool shiftValuesLeft(mValue *dst, mValue *a, mValue *b)
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
    if (t != mValue::UINT) {
        if (t == mValue::INT) {
            if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
                dst->i = ia << ib;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
        dst->u = ua << ub;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool shiftValuesRight(mValue *dst, mValue *a, mValue *b)
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
    if (t != mValue::UINT) {
        if (t == mValue::INT) {
            if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
                dst->i = ia >> ib;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
        dst->u = ua >> ub;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool logicalOrValues(mValue *dst, mValue *a, mValue *b)
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
    if (t != mValue::UINT) {
        if (t == mValue::INT) {
            if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
                dst->i = ia || ib;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
        dst->u = ua || ub;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool logicalAndValues(mValue *dst, mValue *a, mValue *b)
{
    int ia, ib;
    unsigned int ua, ub;
    if (!a || !b) {
        printf("xpVal::logand null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::logand invalid arg\n");
        return false;
    }
    int t = promoteIntType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::logand type promote failed\n");
        return false;
    }
    if (t != mValue::UINT) {
        if (t == mValue::INT) {
            if (getIntInl(a, &ia) && getIntInl(b, &ib)) {
                dst->i = ia && ib;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(a, &ua) && getUIntInl(b, &ub)) {
        dst->u = ua && ub;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

static inline int promoteType(int a, int b)
{
    int t = a;
    if (b > a)
        t = b;
    return t;
}

bool bitwiseAndValues(mValue *dst, mValue *a, mValue *b)
{
    if (!a || !b) {
        printf("xpVal::bitand null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::bitand invalid arg\n");
        return false;
    }
    int t = promoteType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::bitand type promote failed\n");
        return false;
    }
    dst->u = a->u & b->u;
    return true;
}

bool bitwiseOrValues(mValue *dst, mValue *a, mValue *b)
{
    if (!a || !b) {
        printf("xpVal::bitor null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::bitor invalid arg\n");
        return false;
    }
    int t = promoteType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::bitor type promote failed\n");
        return false;
    }
    dst->u = a->u | b->u;
    return true;
}

bool bitwiseXorValues(mValue *dst, mValue *a, mValue *b)
{
    if (!a || !b) {
        printf("xpVal::bitxor null arg passed\n");
        return false;
    }
    if (!isNumType(a->type) || !isNumType(b->type)) {
        printf("xpVal::bitxor invalid arg\n");
        return false;
    }
    int t = promoteType(a->type, b->type);
    dst->type = t;
    if (t == mValue::NONE) {
        printf("xpVal::bitxor type promote failed\n");
        return false;
    }
    dst->u = a->u ^ b->u;
    return true;
}

bool hasOnlyImmediateCharacters(const char *s)
{
    if (!s)
        return false;
    for (; *s; s++) {
        const char *p = g_080b308c;
        while (*p) {
            if (*p == *s)
                break;
            p++;
        }
        if (!*p)
            return false;
    }
    return true;
}

bool mValue::parse(const char *s)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    if (!s)
        return false;
    int dots = 0;
    int digits = 0;
    int rtype = INT;
    int base = 0;
    int value = 0;
    int radix = 10;
    bool isFlt = false;
    bool hasExp = false;
    int expSign = 0;
    const char *start = s;
    if (s[0] == '0' && s[1]) {
        switch (s[1]) {
        case 'x':
        case 'X':
            base = 1;
            radix = 16;
            rtype = g_080b3088 ? FLOAT : UINT;
            start = s + 2;
            break;
        case 'b':
        case 'B':
            radix = 2;
            base = 2;
            start = s + 2;
            break;
        case 'z':
        case 'Z':
            base = 1;
            radix = 16;
            rtype = FLOAT;
            start = s + 2;
            break;
        }
    }
    int skip = 0;
    if (!*start) {
        printf("badly formed literal %s\n", s);
        return false;
    }
    for (const char *p = start; *p; p++) {
        char c = *p;
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (base == 0) {
            if (c == '.') {
                dots++;
                if (isFlt) {
                    printf("error in floating point expression '%s'\n", s);
                    return false;
                }
                if (dots > 1) {
                    printf("unexpected '.' in number\n");
                    return false;
                }
                rtype = FLOAT;
                isFlt = true;
                value = 0;
                digits = 0;
                skip = 1;
            }
            if (c == 'e' && (p[1] == '+' || p[1] == '-')) {
                rtype = FLOAT;
                if (expSign) {
                    printf("already found exponent in %s\n", s);
                    return false;
                }
                if (hasExp) {
                    printf("error in float expression %s\n", s);
                    return false;
                }
                if (isFlt)
                    hasExp = true;
                else {
                    isFlt = true;
                    hasExp = true;
                }
                value = 0;
                digits = 0;
                expSign = (p[1] == '+') ? 1 : -1;
                skip = 2;
            }
        }
        if (c == '_')
            skip = 1;
        if (!skip) {
            const char *d = g_080b3090[base];
            int idx;
            char ch;
            for (idx = 0; (ch = *d) != 0; d++, idx++)
                if (c == ch)
                    break;
            if (!ch)
                return false;
            value = value * radix + idx;
        }
        if (skip)
            skip--;
        else
            digits++;
    }
    if (skip || !digits) {
        reportError("error parsing literal %s\n", s);
        return false;
    }
    if (isFlt)
        f = strtod(s, 0);
    else
        i = value;
    type = rtype;
    return true;
}

void mValue::dump()
{
}

char *mValue::getString()
{
    if (type == STRING)
        return str;
    return 0;
}

bool mValue::makeNumeric()
{
    if (type == STRING) {
        mValue tmp(NONE);
        tmp.parse(str);
        if (isNumType(tmp.type))
            set(&tmp);
    }
    return isNumType(type);
}

char g_080bbda0[4096];
int g_080bcda0;

char *mValue::asChars()
{
    if (g_080bcda0 > 2048)
        g_080bcda0 = 0;
    char *p = g_080bbda0 + g_080bcda0;
    switch (type) {
    case INT:
        formatIntoBuffer(p, "%d", i);
        break;
    case UINT:
        formatIntoBuffer(p, "%u", u);
        break;
    case FLOAT:
        formatIntoBuffer(p, "%1.9e", f);
        break;
    case STRING:
        if (!str) {
            reportError("asChars - but no str for a literal\n");
            return 0;
        }
        formatIntoBuffer(p, "%s", str);
        break;
    }
    g_080bcda0 += mstrlen(p) + 1;
    return p;
}

bool sameImmediateValue(mValue *a, mValue *b)
{
    unsigned int za = 0;
    unsigned int zb = 0;
    unsigned int *pa = &za;
    unsigned int *pb = &zb;
    if (a && b) {
        if (a->type != b->type)
            return false;
        if (a->type == mValue::STRING)
            return asciiCaseEqual(a->str, b->str);
    }
    if (a && a->type == mValue::STRING)
        return false;
    if (b && b->type == mValue::STRING)
        return false;
    if (a && isNumType(a->type))
        pa = &a->u;
    if (b && isNumType(b->type))
        pb = &b->u;
    return *pa == *pb;
}
