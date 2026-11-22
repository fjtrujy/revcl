// VU component-mask string helpers, CIE group 3.
#include "VUStr.h"
#include "os.h"

bool parseTrailingComponentMask(const char *componentText, int *mask, bool *bad)
{
    const char *componentCursor = componentText;
    *mask = 0;
    *bad = false;
    if (!componentText || !*componentText)
        return false;
    while (*componentCursor)
        componentCursor++;
    componentCursor--;
    while (componentCursor != componentText && *componentCursor != '.')
        componentCursor--;
    if (*componentCursor == '.') {
        int decodedMask = decodeComponentMaskLetters(componentCursor + 1);
        *mask = decodedMask;
        if (decodedMask == -1) {
            *bad = true;
            return false;
        }
        return true;
    }
    return false;
}

bool parseBracketedComponentMask(const char *componentText, int *mask)
{
    *mask = 0;
    const char *componentCursor = componentText;
    if (!componentText || !*componentText)
        return false;
    while (*componentCursor)
        componentCursor++;
    componentCursor--;
    if (*componentCursor != ']')
        return false;
    componentCursor--;
    *mask = 0;
    for (; componentCursor != componentText && *componentCursor != '['; componentCursor--) {
        switch (*componentCursor) {
        case 'x': case 'X': *mask |= 1; break;
        case 'y': case 'Y': *mask |= 2; break;
        case 'z': case 'Z': *mask |= 4; break;
        case 'w': case 'W': *mask |= 8; break;
        default: return false;
        }
    }
    return true;
}

int decodeComponentMaskLetters(const char *componentText)
{
    int mask = 0;
    while (*componentText) {
        char componentLetter = *componentText;
        int bit = 0;
        if (componentLetter == 'x' || componentLetter == 'X') bit = 1;
        if (componentLetter == 'y' || componentLetter == 'Y') bit = 2;
        if (componentLetter == 'z' || componentLetter == 'Z') bit = 4;
        if (componentLetter == 'w' || componentLetter == 'W') bit = 8;
        if (!bit)
            return -1;
        if (mask & bit)
            return -1;
        mask |= bit;
        componentText++;
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
    char *outputCursor = start;
    if (mask & 1) *outputCursor++ = 'X';
    if (mask & 2) *outputCursor++ = 'Y';
    if (mask & 4) *outputCursor++ = 'Z';
    if (mask & 8) *outputCursor++ = 'W';
    *outputCursor = 0;
    componentMaskStringOffset = outputCursor - componentMaskTextBuffer + 1;
    return start;
}

inline char *VUStr_dot_mask(char *buf, int mask)
{
    buf[0] = '.';
    checkedStringCopy(buf + 1, formatComponentMaskLetters(mask));
    return buf;
}

char *formatDottedComponentMask(int mask)
{
    return VUStr_dot_mask(dottedComponentMaskBuffer, mask);
}

int readTrailingComponentMask(const char *componentText)
{
    int mask = 0;
    if (!componentText || !*componentText)
        return 0;
    const char *componentCursor = componentText;
    while (*componentCursor)
        componentCursor++;
    for (componentCursor--; componentCursor != componentText - 1; componentCursor--) {
        char componentLetter = *componentCursor;
        int bit = 0;
        if (componentLetter == 'x' || componentLetter == 'X') bit = 1;
        if (componentLetter == 'y' || componentLetter == 'Y') bit = 2;
        if (componentLetter == 'z' || componentLetter == 'Z') bit = 4;
        if (componentLetter == 'w' || componentLetter == 'W') bit = 8;
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

int lastComponentBit(const char *componentText)
{
    int lastComponentMask = 0;
    if (!componentText || !*componentText)
        return 0;
    const char *componentCursor = componentText;
    while (*componentCursor)
        componentCursor++;
    switch (componentCursor[-1]) {
    case 'x': case 'X': lastComponentMask = 1; break;
    case 'y': case 'Y': lastComponentMask = 2; break;
    case 'z': case 'Z': lastComponentMask = 4; break;
    case 'w': case 'W': lastComponentMask = 8; break;
    }
    return lastComponentMask;
}

int removeInstructionModifierGroups(char *modifierText)
{
    int mods = 0;
    if (modifierText) {
        char *destinationBuffer = modifierText;
        char *src = modifierText;
        while (*src) {
            while (*src) {
                if (*src == '[')
                    break;
                *destinationBuffer++ = *src++;
            }
            if (*src == '[') {
                char *modifierCursor = src + 1;
                int modifierMask = 0;
                bool validModifierGroup = true;
                while (*modifierCursor && *modifierCursor != ']') {
                    switch (*modifierCursor) {
                    case 'E': case 'e': modifierMask |= 2; break;
                    case 'D': case 'd': modifierMask |= 4; break;
                    case 'M': case 'm': modifierMask |= 1; break;
                    case 'T': case 't': modifierMask |= 8; break;
                    default: validModifierGroup = false; break;
                    }
                    modifierCursor++;
                }
                if (*modifierCursor != ']')
                    validModifierGroup = false;
                if (validModifierGroup) {
                    mods |= modifierMask;
                    src = modifierCursor + 1;
                } else {
                    reportWarning("possibly misformed instruction modifier detected\n");
                    *destinationBuffer++ = *src++;
                }
            }
        }
        *destinationBuffer = 0;
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
    char *outputCursor = instructionModifierTextBuffer;
    if (mods)
        *outputCursor++ = '[';
    if (mods & 4) *outputCursor++ = 'D';
    if (mods & 2) *outputCursor++ = 'E';
    if (mods & 8) *outputCursor++ = 'T';
    if (mods & 1) *outputCursor++ = 'M';
    if (mods & ~15) *outputCursor++ = '?';
    if (mods) *outputCursor++ = ']';
    *outputCursor = 0;
    return instructionModifierTextBuffer;
}
