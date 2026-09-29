// VU component-mask string helpers, CIE group 3.
#include "VUStr.h"
#include "os.h"

bool func_08051f08(const char *s, int *mask, bool *bad)
{
    const char *p = s;
    *mask = 0;
    *bad = false;
    if (!s || !*s)
        return false;
    while (*p)
        p++;
    p--;
    while (p != s && *p != '.')
        p--;
    if (*p == '.') {
        int m = func_08051ff0(p + 1);
        *mask = m;
        if (m == -1) {
            *bad = true;
            return false;
        }
        return true;
    }
    return false;
}

bool func_08051f78(const char *s, int *mask)
{
    *mask = 0;
    const char *p = s;
    if (!s || !*s)
        return false;
    while (*p)
        p++;
    p--;
    if (*p != ']')
        return false;
    p--;
    *mask = 0;
    for (; p != s && *p != '['; p--) {
        switch (*p) {
        case 'x': case 'X': *mask |= 1; break;
        case 'y': case 'Y': *mask |= 2; break;
        case 'z': case 'Z': *mask |= 4; break;
        case 'w': case 'W': *mask |= 8; break;
        default: return false;
        }
    }
    return true;
}

int func_08051ff0(const char *s)
{
    int mask = 0;
    while (*s) {
        char c = *s;
        int bit = 0;
        if (c == 'x' || c == 'X') bit = 1;
        if (c == 'y' || c == 'Y') bit = 2;
        if (c == 'z' || c == 'Z') bit = 4;
        if (c == 'w' || c == 'W') bit = 8;
        if (!bit)
            return -1;
        if (mask & bit)
            return -1;
        mask |= bit;
        s++;
    }
    return mask;
}

int g_080b3240 = 0;
char g_080bcdc0[32];
char g_080bcde0[32];

char *func_08052064(int mask)
{
    if (g_080b3240 > 24)
        g_080b3240 = 0;
    char *start = g_080bcde0 + g_080b3240;
    char *p = start;
    if (mask & 1) *p++ = 'X';
    if (mask & 2) *p++ = 'Y';
    if (mask & 4) *p++ = 'Z';
    if (mask & 8) *p++ = 'W';
    *p = 0;
    g_080b3240 = p - g_080bcde0 + 1;
    return start;
}

inline char *VUStr_dot_mask(char *buf, int mask)
{
    buf[0] = '.';
    mstrcpy(buf + 1, func_08052064(mask));
    return buf;
}

char *func_080520d4(int mask)
{
    return VUStr_dot_mask(g_080bcdc0, mask);
}

int func_08052100(const char *s)
{
    int mask = 0;
    if (!s || !*s)
        return 0;
    const char *p = s;
    while (*p)
        p++;
    for (p--; p != s - 1; p--) {
        char c = *p;
        int bit = 0;
        if (c == 'x' || c == 'X') bit = 1;
        if (c == 'y' || c == 'Y') bit = 2;
        if (c == 'z' || c == 'Z') bit = 4;
        if (c == 'w' || c == 'W') bit = 8;
        if (!bit)
            return mask;
        if (mask & bit) {
            func_0804ac7c("VUSTR:: badly formed mod bits string!\n");
            return -1;
        }
        mask |= bit;
    }
    return mask;
}

int func_0805219c(const char *s)
{
    int r = 0;
    if (!s || !*s)
        return 0;
    const char *p = s;
    while (*p)
        p++;
    switch (p[-1]) {
    case 'x': case 'X': r = 1; break;
    case 'y': case 'Y': r = 2; break;
    case 'z': case 'Z': r = 4; break;
    case 'w': case 'W': r = 8; break;
    }
    return r;
}
