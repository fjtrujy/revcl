// Instruction list helpers: ToDo / Done / Linear lists (CIE group 1, after mList.cpp).

#include <stdio.h>
#include <stdlib.h>
#include "Block.h"
#include "Instr.h"
#include "mValue.h"
#include "os.h"

bool UnlinkFromToDo(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromToDo - passed a null pointer\n");
        return false;
    }
    if (!blk->pendingInstructions.count) {
        reportError("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->pendingLink.nextInstruction)
        instruction->pendingLink.nextInstruction->pendingLink.previousInstruction = instruction->pendingLink.previousInstruction;
    else if (instruction == blk->pendingInstructions.tail)
        blk->pendingInstructions.tail = instruction->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction)
        instruction->pendingLink.previousInstruction->pendingLink.nextInstruction = instruction->pendingLink.nextInstruction;
    else if (instruction == blk->pendingInstructions.head)
        blk->pendingInstructions.head = instruction->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    blk->pendingInstructions.count--;
    instruction->pendingLink.previousInstruction = 0;
    instruction->pendingLink.nextInstruction = 0;
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!blk->completedInstructions.count) {
        reportError("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->pendingLink.nextInstruction)
        instruction->pendingLink.nextInstruction->pendingLink.previousInstruction = instruction->pendingLink.previousInstruction;
    else if (instruction == blk->completedInstructions.tail)
        blk->completedInstructions.tail = instruction->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction)
        instruction->pendingLink.previousInstruction->pendingLink.nextInstruction = instruction->pendingLink.nextInstruction;
    else if (instruction == blk->completedInstructions.head)
        blk->completedInstructions.head = instruction->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    blk->completedInstructions.count--;
    instruction->pendingLink.previousInstruction = 0;
    instruction->pendingLink.nextInstruction = 0;
    return true;
}

bool UnlinkFromLinear(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!blk->linearInstructions.count) {
        reportError("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->linearLink.nextInstruction)
        instruction->linearLink.nextInstruction->linearLink.previousInstruction = instruction->linearLink.previousInstruction;
    else if (instruction == blk->linearInstructions.tail)
        blk->linearInstructions.tail = instruction->linearLink.previousInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction)
        instruction->linearLink.previousInstruction->linearLink.nextInstruction = instruction->linearLink.nextInstruction;
    else if (instruction == blk->linearInstructions.head)
        blk->linearInstructions.head = instruction->linearLink.nextInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    blk->linearInstructions.count--;
    instruction->linearLink.previousInstruction = 0;
    instruction->linearLink.nextInstruction = 0;
    return true;
}

bool AddToDo_Head(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
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
        instruction->pendingLink.nextInstruction = blk->pendingInstructions.head;
        blk->pendingInstructions.head->pendingLink.previousInstruction = instruction;
        blk->pendingInstructions.head = instruction;
    } else {
        if (blk->pendingInstructions.count) {
            reportError("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        blk->pendingInstructions.tail = instruction;
        blk->pendingInstructions.head = instruction;
    }
    blk->pendingInstructions.count++;
    return true;
}

bool AddDone_Head(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddDone_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
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
        instruction->pendingLink.nextInstruction = blk->completedInstructions.head;
        blk->completedInstructions.head->pendingLink.previousInstruction = instruction;
        blk->completedInstructions.head = instruction;
    } else {
        if (blk->completedInstructions.count) {
            reportError("AddDone_Headno list head but count not zero\n");
            return false;
        }
        blk->completedInstructions.tail = instruction;
        blk->completedInstructions.head = instruction;
    }
    blk->completedInstructions.count++;
    return true;
}

bool AddLinear_Head(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddLinear_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction || instruction->linearLink.nextInstruction) {
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
        instruction->linearLink.nextInstruction = blk->linearInstructions.head;
        blk->linearInstructions.head->linearLink.previousInstruction = instruction;
        blk->linearInstructions.head = instruction;
    } else {
        if (blk->linearInstructions.count) {
            reportError("AddLinear_Headno list head but count not zero\n");
            return false;
        }
        blk->linearInstructions.tail = instruction;
        blk->linearInstructions.head = instruction;
    }
    blk->linearInstructions.count++;
    return true;
}

bool AddToDo_Tail(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddToDo_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
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
        instruction->pendingLink.previousInstruction = blk->pendingInstructions.tail;
        blk->pendingInstructions.tail->pendingLink.nextInstruction = instruction;
        blk->pendingInstructions.tail = instruction;
    } else {
        if (blk->pendingInstructions.count) {
            reportError("AddToDo_Tailno list head but count not zero\n");
            return false;
        }
        blk->pendingInstructions.tail = instruction;
        blk->pendingInstructions.head = instruction;
    }
    blk->pendingInstructions.count++;
    return true;
}

bool AddDone_Tail(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddDone_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
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
        instruction->pendingLink.previousInstruction = blk->completedInstructions.tail;
        blk->completedInstructions.tail->pendingLink.nextInstruction = instruction;
        blk->completedInstructions.tail = instruction;
    } else {
        if (blk->completedInstructions.count) {
            reportError("AddDone_Tailno list head but count not zero\n");
            return false;
        }
        blk->completedInstructions.tail = instruction;
        blk->completedInstructions.head = instruction;
    }
    blk->completedInstructions.count++;
    return true;
}

bool AddLinear_Tail(Block *blk, Instr *instruction)
{
    if (!instruction) {
        reportError("AddLinear_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction || instruction->linearLink.nextInstruction) {
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
        instruction->linearLink.previousInstruction = blk->linearInstructions.tail;
        blk->linearInstructions.tail->linearLink.nextInstruction = instruction;
        blk->linearInstructions.tail = instruction;
    } else {
        if (blk->linearInstructions.count) {
            reportError("AddLinear_Tailno list head but count not zero\n");
            return false;
        }
        blk->linearInstructions.tail = instruction;
        blk->linearInstructions.head = instruction;
    }
    blk->linearInstructions.count++;
    return true;
}

bool AddLinear_After(Block *blk, Instr *after, Instr *instruction)
{
    if (!instruction) {
        reportError("AddLinear_After - passed a null in_a pointer\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction || instruction->linearLink.nextInstruction) {
        reportError("AddLinear_After - node already appears linked\n");
        return false;
    }
    if (after) {
        instruction->linearLink.nextInstruction = after->linearLink.nextInstruction;
        instruction->linearLink.previousInstruction = after;
        after->linearLink.nextInstruction = instruction;
    } else {
        instruction->linearLink.nextInstruction = blk->linearInstructions.head;
        blk->linearInstructions.head = instruction;
    }
    if (instruction->linearLink.nextInstruction)
        instruction->linearLink.nextInstruction->linearLink.previousInstruction = instruction;
    else
        blk->linearInstructions.tail = instruction;
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
    int bitCount = 0;
    while (x) {
        bitCount += x & 1;
        x >>= 1;
    }
    return bitCount;
}

const char *tokenAllowedCharacters =
    "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?";
const char *identifierAllowedCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@";

bool isTokenAlphabetCharacter(char c)
{
    const char *cursor = tokenAllowedCharacters;
    char matchedChar;
    while ((matchedChar = *cursor) != 0 && matchedChar != c)
        cursor++;
    return matchedChar != 0;
}

bool isIdentifierAlphabetCharacter(char c)
{
    const char *cursor = identifierAllowedCharacters;
    char matchedChar;
    while ((matchedChar = *cursor) != 0 && matchedChar != c)
        cursor++;
    return matchedChar != 0;
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
    char *cursor = s;
    if (!s || !*s)
        return;
    while (*cursor)
        cursor++;
    cursor--;
    while (cursor != s && *cursor != '.')
        cursor--;
    if (*cursor == '.')
        *cursor = 0;
}

void removeTrailingComponentSuffix(char *s)
{
    char *cursor = s;
    if (!s || !*s)
        return;
    while (*cursor)
        cursor++;
    for (--cursor; cursor != s && (*cursor == 'x' || *cursor == 'X' || *cursor == 'y' || *cursor == 'Y' ||
                        *cursor == 'z' || *cursor == 'Z' || *cursor == 'w' || *cursor == 'W'); --cursor)
        ;
    cursor[1] = 0;
    if (*cursor == '.')
        *cursor = 0;
}

char *dupString(const char *s)
{
    if (!s) {
        reportError("dupString: passed null ptr\n");
        return 0;
    }
    const char *sourceEnd = s;
    while (*sourceEnd)
        sourceEnd++;
    char *duplicate = new char[sourceEnd - s + 1];
    if (!duplicate) {
        reportError("dupString: new failed to get ram\n");
        reportError("aborting.. sorry\n");
        exitAfterCleanup(-1);
    }
    char *outputCursor = duplicate;
    while (*s)
        *outputCursor++ = *s++;
    *outputCursor = 0;
    return duplicate;
}

char *dupLabelString(const char *s)
{
    if (!s) {
        reportError("dupLabelString: null ptr input\n");
        return 0;
    }
    const char *sourceEnd = s;
    while (*sourceEnd)
        sourceEnd++;
    char *duplicate = new char[sourceEnd - s + 1];
    if (!duplicate) {
        reportError("dupLabelString: failed to get ram input\n");
        return 0;
    }
    char *outputCursor = duplicate;
    while (*s && *s != ':')
        *outputCursor++ = *s++;
    *outputCursor = 0;
    return duplicate;
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
    char *outputCursor = p;
    for (; n; n--)
        *outputCursor++ = 0;
}

int mstrlen(const char *s)
{
    if (!s) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    const char *stringEnd = s;
    while (*stringEnd)
        stringEnd++;
    return stringEnd - s;
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
    const char *sourceCursor = src;
    char *destinationCursor = dst;
    while (*sourceCursor)
        *destinationCursor++ = *sourceCursor++;
    *destinationCursor = 0;
    return dst;
}

char *mprefix_del(const char *a, const char *b)
{
    if (!a || !b) {
        reportError("mprefix_del - not str\n");
        return 0;
    }
    const char *stringEnd = a;
    while (*stringEnd)
        stringEnd++;
    int prefixLength = stringEnd - a;
    if (!b) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    stringEnd = b;
    while (*stringEnd)
        stringEnd++;
    char *joinedName = new char[prefixLength + (stringEnd - b) + 2];
    const char *sourceCursor = a;
    char *outputCursor = joinedName;
    if (!joinedName) {
        reportError("failed to get a string\n");
        return 0;
    }
    while (*sourceCursor)
        *outputCursor++ = *sourceCursor++;
    *outputCursor++ = '_';
    sourceCursor = b;
    while (*sourceCursor)
        *outputCursor++ = *sourceCursor++;
    *outputCursor = 0;
    return joinedName;
}
