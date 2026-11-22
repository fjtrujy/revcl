// VU component-mask string helpers, CIE group 3.
#include "VUStr.h"
#include "os.h"

bool parseTrailingComponentMask(const char *s, int *mask, bool *bad)
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
        int m = decodeComponentMaskLetters(p + 1);
        *mask = m;
        if (m == -1) {
            *bad = true;
            return false;
        }
        return true;
    }
    return false;
}

bool parseBracketedComponentMask(const char *s, int *mask)
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

int decodeComponentMaskLetters(const char *s)
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

int componentMaskStringOffset = 0;
char dottedComponentMaskBuffer[32];
char componentMaskTextBuffer[32];

char *formatComponentMaskLetters(int mask)
{
    if (componentMaskStringOffset > 24)
        componentMaskStringOffset = 0;
    char *start = componentMaskTextBuffer + componentMaskStringOffset;
    char *p = start;
    if (mask & 1) *p++ = 'X';
    if (mask & 2) *p++ = 'Y';
    if (mask & 4) *p++ = 'Z';
    if (mask & 8) *p++ = 'W';
    *p = 0;
    componentMaskStringOffset = p - componentMaskTextBuffer + 1;
    return start;
}

inline char *VUStr_dot_mask(char *buf, int mask)
{
    buf[0] = '.';
    mstrcpy(buf + 1, formatComponentMaskLetters(mask));
    return buf;
}

char *formatDottedComponentMask(int mask)
{
    return VUStr_dot_mask(dottedComponentMaskBuffer, mask);
}

int readTrailingComponentMask(const char *s)
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
            reportWarning("VUSTR:: badly formed mod bits string!\n");
            return -1;
        }
        mask |= bit;
    }
    return mask;
}

int lastComponentBit(const char *s)
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

int removeInstructionModifierGroups(char *s)
{
    int mods = 0;
    if (s) {
        char *dst = s;
        char *src = s;
        while (*src) {
            while (*src) {
                if (*src == '[')
                    break;
                *dst++ = *src++;
            }
            if (*src == '[') {
                char *p = src + 1;
                int m = 0;
                bool ok = true;
                while (*p && *p != ']') {
                    switch (*p) {
                    case 'E': case 'e': m |= 2; break;
                    case 'D': case 'd': m |= 4; break;
                    case 'M': case 'm': m |= 1; break;
                    case 'T': case 't': m |= 8; break;
                    default: ok = false; break;
                    }
                    p++;
                }
                if (*p != ']')
                    ok = false;
                if (ok) {
                    mods |= m;
                    src = p + 1;
                } else {
                    reportWarning("possibly misformed instruction modifier detected\n");
                    *dst++ = *src++;
                }
            }
        }
        *dst = 0;
    }
    return mods;
}

bool areInstructionModifiersAllowed(int mods, unsigned char mode, bool flag)
{
    int allowed = 0;
    switch (mode) {
    case 1:
        allowed = flag ? 13 : 12;
        break;
    case 3:
        allowed = 13;
        break;
    default:
        reportWarning("bad mode to ask about mods\n");
        break;
    }
    return (~allowed & mods) == 0;
}

char instructionModifierTextBuffer[32];

char *formatInstructionModifiers(int mods)
{
    char *p = instructionModifierTextBuffer;
    if (mods)
        *p++ = '[';
    if (mods & 4) *p++ = 'D';
    if (mods & 2) *p++ = 'E';
    if (mods & 8) *p++ = 'T';
    if (mods & 1) *p++ = 'M';
    if (mods & ~15) *p++ = '?';
    if (mods) *p++ = ']';
    *p = 0;
    return instructionModifierTextBuffer;
}
