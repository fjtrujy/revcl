// Instruction list helpers: ToDo / Done / Linear lists (CIE group 1, after mList.cpp).

#include <stdio.h>
#include <stdlib.h>
#include "Block.h"
#include "Instr.h"
#include "mValue.h"
#include "os.h"

bool unlinkFromPendingInstructions(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromToDo - passed a null pointer\n");
        return false;
    }
    if (!block->pendingInstructions.count) {
        reportError("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->pendingLink.nextInstruction)
        instruction->pendingLink.nextInstruction->pendingLink.previousInstruction = instruction->pendingLink.previousInstruction;
    else if (instruction == block->pendingInstructions.tail)
        block->pendingInstructions.tail = instruction->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction)
        instruction->pendingLink.previousInstruction->pendingLink.nextInstruction = instruction->pendingLink.nextInstruction;
    else if (instruction == block->pendingInstructions.head)
        block->pendingInstructions.head = instruction->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    block->pendingInstructions.count--;
    instruction->pendingLink.previousInstruction = 0;
    instruction->pendingLink.nextInstruction = 0;
    return true;
}

bool unlinkFromCompletedInstructions(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!block->completedInstructions.count) {
        reportError("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->pendingLink.nextInstruction)
        instruction->pendingLink.nextInstruction->pendingLink.previousInstruction = instruction->pendingLink.previousInstruction;
    else if (instruction == block->completedInstructions.tail)
        block->completedInstructions.tail = instruction->pendingLink.previousInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction)
        instruction->pendingLink.previousInstruction->pendingLink.nextInstruction = instruction->pendingLink.nextInstruction;
    else if (instruction == block->completedInstructions.head)
        block->completedInstructions.head = instruction->pendingLink.nextInstruction;
    else {
        reportError("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    block->completedInstructions.count--;
    instruction->pendingLink.previousInstruction = 0;
    instruction->pendingLink.nextInstruction = 0;
    return true;
}

bool unlinkFromLinearInstructions(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!block->linearInstructions.count) {
        reportError("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (instruction->linearLink.nextInstruction)
        instruction->linearLink.nextInstruction->linearLink.previousInstruction = instruction->linearLink.previousInstruction;
    else if (instruction == block->linearInstructions.tail)
        block->linearInstructions.tail = instruction->linearLink.previousInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction)
        instruction->linearLink.previousInstruction->linearLink.nextInstruction = instruction->linearLink.nextInstruction;
    else if (instruction == block->linearInstructions.head)
        block->linearInstructions.head = instruction->linearLink.nextInstruction;
    else {
        reportError("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    block->linearInstructions.count--;
    instruction->linearLink.previousInstruction = 0;
    instruction->linearLink.nextInstruction = 0;
    return true;
}

bool prependPendingInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
        reportError("AddToDo_Head - node already appears linked\n");
        return false;
    }
    if (block->pendingInstructions.head) {
        if (!block->pendingInstructions.tail) {
            reportError("AddToDo_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->pendingInstructions.count) {
            reportError("AddToDo_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->pendingLink.nextInstruction = block->pendingInstructions.head;
        block->pendingInstructions.head->pendingLink.previousInstruction = instruction;
        block->pendingInstructions.head = instruction;
    } else {
        if (block->pendingInstructions.count) {
            reportError("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        block->pendingInstructions.tail = instruction;
        block->pendingInstructions.head = instruction;
    }
    block->pendingInstructions.count++;
    return true;
}

bool prependCompletedInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddDone_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
        reportError("AddDone_Head - node already appears linked\n");
        return false;
    }
    if (block->completedInstructions.head) {
        if (!block->completedInstructions.tail) {
            reportError("AddDone_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->completedInstructions.count) {
            reportError("AddDone_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->pendingLink.nextInstruction = block->completedInstructions.head;
        block->completedInstructions.head->pendingLink.previousInstruction = instruction;
        block->completedInstructions.head = instruction;
    } else {
        if (block->completedInstructions.count) {
            reportError("AddDone_Headno list head but count not zero\n");
            return false;
        }
        block->completedInstructions.tail = instruction;
        block->completedInstructions.head = instruction;
    }
    block->completedInstructions.count++;
    return true;
}

bool prependLinearInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddLinear_Head - passed a null pointer\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction || instruction->linearLink.nextInstruction) {
        reportError("AddLinear_Head - node already appears linked\n");
        return false;
    }
    if (block->linearInstructions.head) {
        if (!block->linearInstructions.tail) {
            reportError("AddLinear_Headlist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->linearInstructions.count) {
            reportError("AddLinear_Headlist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->linearLink.nextInstruction = block->linearInstructions.head;
        block->linearInstructions.head->linearLink.previousInstruction = instruction;
        block->linearInstructions.head = instruction;
    } else {
        if (block->linearInstructions.count) {
            reportError("AddLinear_Headno list head but count not zero\n");
            return false;
        }
        block->linearInstructions.tail = instruction;
        block->linearInstructions.head = instruction;
    }
    block->linearInstructions.count++;
    return true;
}

bool appendPendingInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddToDo_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
        reportError("AddToDo_Tail - node already appears linked\n");
        return false;
    }
    if (block->pendingInstructions.head) {
        if (!block->pendingInstructions.tail) {
            reportError("AddToDo_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->pendingInstructions.count) {
            reportError("AddToDo_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->pendingLink.previousInstruction = block->pendingInstructions.tail;
        block->pendingInstructions.tail->pendingLink.nextInstruction = instruction;
        block->pendingInstructions.tail = instruction;
    } else {
        if (block->pendingInstructions.count) {
            reportError("AddToDo_Tailno list head but count not zero\n");
            return false;
        }
        block->pendingInstructions.tail = instruction;
        block->pendingInstructions.head = instruction;
    }
    block->pendingInstructions.count++;
    return true;
}

bool appendCompletedInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddDone_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->pendingLink.previousInstruction || instruction->pendingLink.nextInstruction) {
        reportError("AddDone_Tail - node already appears linked\n");
        return false;
    }
    if (block->completedInstructions.head) {
        if (!block->completedInstructions.tail) {
            reportError("AddDone_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->completedInstructions.count) {
            reportError("AddDone_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->pendingLink.previousInstruction = block->completedInstructions.tail;
        block->completedInstructions.tail->pendingLink.nextInstruction = instruction;
        block->completedInstructions.tail = instruction;
    } else {
        if (block->completedInstructions.count) {
            reportError("AddDone_Tailno list head but count not zero\n");
            return false;
        }
        block->completedInstructions.tail = instruction;
        block->completedInstructions.head = instruction;
    }
    block->completedInstructions.count++;
    return true;
}

bool appendLinearInstruction(Block *block, Instr *instruction)
{
    if (!instruction) {
        reportError("AddLinear_Tail - passed a null pointer\n");
        return false;
    }
    if (instruction->linearLink.previousInstruction || instruction->linearLink.nextInstruction) {
        reportError("AddLinear_Tail - node already appears linked\n");
        return false;
    }
    if (block->linearInstructions.head) {
        if (!block->linearInstructions.tail) {
            reportError("AddLinear_Taillist had no first, but last set!\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (!block->linearInstructions.count) {
            reportError("AddLinear_Taillist had no head or tail, but count not zero\n");
            exitAfterCleanup(-1);
            return false;
        }
        instruction->linearLink.previousInstruction = block->linearInstructions.tail;
        block->linearInstructions.tail->linearLink.nextInstruction = instruction;
        block->linearInstructions.tail = instruction;
    } else {
        if (block->linearInstructions.count) {
            reportError("AddLinear_Tailno list head but count not zero\n");
            return false;
        }
        block->linearInstructions.tail = instruction;
        block->linearInstructions.head = instruction;
    }
    block->linearInstructions.count++;
    return true;
}

bool insertLinearInstructionAfter(Block *block, Instr *after, Instr *instruction)
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
        instruction->linearLink.nextInstruction = block->linearInstructions.head;
        block->linearInstructions.head = instruction;
    }
    if (instruction->linearLink.nextInstruction)
        instruction->linearLink.nextInstruction->linearLink.previousInstruction = instruction;
    else
        block->linearInstructions.tail = instruction;
    block->linearInstructions.count++;
    return true;
}

bool equalAsciiCaseInsensitive(char firstCharacter, char secondCharacter)
{
    if (firstCharacter >= 'a' && firstCharacter <= 'z')
        firstCharacter -= 32;
    if (secondCharacter >= 'a' && secondCharacter <= 'z')
        secondCharacter -= 32;
    return firstCharacter == secondCharacter;
}

int countSetBits(unsigned int mask)
{
    int bitCount = 0;
    while (mask) {
        bitCount += mask & 1;
        mask >>= 1;
    }
    return bitCount;
}

const char *tokenAllowedCharacters =
    "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?";
const char *identifierAllowedCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@";

bool isTokenAlphabetCharacter(char character)
{
    const char *cursor = tokenAllowedCharacters;
    char matchedChar;
    while ((matchedChar = *cursor) != 0 && matchedChar != character)
        cursor++;
    return matchedChar != 0;
}

bool isIdentifierAlphabetCharacter(char character)
{
    const char *cursor = identifierAllowedCharacters;
    char matchedChar;
    while ((matchedChar = *cursor) != 0 && matchedChar != character)
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

bool equalExactStrings(const char *leftText, const char *rightText)
{
    if (!leftText || !rightText)
        return false;
    while (*leftText && *rightText && *leftText == *rightText) {
        leftText++;
        rightText++;
    }
    if (*leftText || *rightText)
        return false;
    return true;
}

void removeLastDotSuffix(char *text)
{
    char *cursor = text;
    if (!text || !*text)
        return;
    while (*cursor)
        cursor++;
    cursor--;
    while (cursor != text && *cursor != '.')
        cursor--;
    if (*cursor == '.')
        *cursor = 0;
}

void removeTrailingComponentSuffix(char *text)
{
    char *cursor = text;
    if (!text || !*text)
        return;
    while (*cursor)
        cursor++;
    for (--cursor; cursor != text && (*cursor == 'x' || *cursor == 'X' || *cursor == 'y' || *cursor == 'Y' ||
                        *cursor == 'z' || *cursor == 'Z' || *cursor == 'w' || *cursor == 'W'); --cursor)
        ;
    cursor[1] = 0;
    if (*cursor == '.')
        *cursor = 0;
}

char *dupString(const char *source)
{
    if (!source) {
        reportError("dupString: passed null ptr\n");
        return 0;
    }
    const char *sourceEnd = source;
    while (*sourceEnd)
        sourceEnd++;
    char *duplicate = new char[sourceEnd - source + 1];
    if (!duplicate) {
        reportError("dupString: new failed to get ram\n");
        reportError("aborting.. sorry\n");
        exitAfterCleanup(-1);
    }
    char *outputCursor = duplicate;
    while (*source)
        *outputCursor++ = *source++;
    *outputCursor = 0;
    return duplicate;
}

char *dupLabelString(const char *source)
{
    if (!source) {
        reportError("dupLabelString: null ptr input\n");
        return 0;
    }
    const char *sourceEnd = source;
    while (*sourceEnd)
        sourceEnd++;
    char *duplicate = new char[sourceEnd - source + 1];
    if (!duplicate) {
        reportError("dupLabelString: failed to get ram input\n");
        return 0;
    }
    char *outputCursor = duplicate;
    while (*source && *source != ':')
        *outputCursor++ = *source++;
    *outputCursor = 0;
    return duplicate;
}

void clearRawBytes(char *buffer, int byteCount)
{
    if (!buffer) {
        reportError("asked to zero memory location 0\n");
        exitAfterCleanup(-1);
    }
    if (byteCount < 0) {
        reportError("asked to zero -ve size\n");
        exitAfterCleanup(-1);
    }
    char *outputCursor = buffer;
    for (; byteCount; byteCount--)
        *outputCursor++ = 0;
}

int checkedStringLength(const char *text)
{
    if (!text) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    const char *stringEnd = text;
    while (*stringEnd)
        stringEnd++;
    return stringEnd - text;
}

char *checkedStringCopy(char *destinationBuffer, const char *src)
{
    if (!destinationBuffer) {
        reportError("mstrcpy passed null dst ptr\n");
        exitAfterCleanup(-1);
    }
    if (!src) {
        reportError("mstrcpy passed null src ptr\n");
        exitAfterCleanup(-1);
    }
    const char *sourceCursor = src;
    char *destinationCursor = destinationBuffer;
    while (*sourceCursor)
        *destinationCursor++ = *sourceCursor++;
    *destinationCursor = 0;
    return destinationBuffer;
}

char *joinNameWithPrefix(const char *firstPart, const char *secondPart)
{
    if (!firstPart || !secondPart) {
        reportError("mprefix_del - not str\n");
        return 0;
    }
    const char *stringEnd = firstPart;
    while (*stringEnd)
        stringEnd++;
    int prefixLength = stringEnd - firstPart;
    if (!secondPart) {
        reportError("mstrlen passed null ptr\n");
        exitAfterCleanup(-1);
    }
    stringEnd = secondPart;
    while (*stringEnd)
        stringEnd++;
    char *joinedName = new char[prefixLength + (stringEnd - secondPart) + 2];
    const char *sourceCursor = firstPart;
    char *outputCursor = joinedName;
    if (!joinedName) {
        reportError("failed to get a string\n");
        return 0;
    }
    while (*sourceCursor)
        *outputCursor++ = *sourceCursor++;
    *outputCursor++ = '_';
    sourceCursor = secondPart;
    while (*sourceCursor)
        *outputCursor++ = *sourceCursor++;
    *outputCursor = 0;
    return joinedName;
}
