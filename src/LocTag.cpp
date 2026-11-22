// lineEntry (LocTag) methods, CIE group 3.
#include "Parser.h"
#include "os.h"

bool isLocTagValid(LocTag *t)
{
    return (t->lineText && t->tokenCount) || (!t->lineText && !t->tokenCount);
}

int getLineTokenCount(LocTag *t)
{
    return t->tokenCount;
}


extern const char locTagInvalidCommentError[];
extern const char locTagTooManyTokensError[];
extern const char locTagNonNullTeError[];
extern const char locTagNullTeError[];
extern const char locTagNullInPError[];
extern const char locTagNullLineDataError[];
extern const char locTagGetTokenNullError[];
extern const char locTagDumpNoPtrError[];
extern const char locTagInvalidDumpError[];

char *LocTag::tokenizeSourceLine(int *commentMode, char *input)
{
    if (!lineText) {
        reportError(locTagNullLineDataError);
        return 0;
    }
    if (!input) {
        reportError(locTagNullInPError);
        return 0;
    }
    if (*commentMode != 0 && *commentMode != 2) {
        reportError(locTagInvalidCommentError);
        return 0;
    }
    char *readCursor = input;
    int tokenState = 1;
    tokenCount = 0;
    int processedCharCount = 0;
    TokenEntry *currentToken = 0;
    TokenEntry *previousToken = 0;
    char *writeCursor = lineText;
    while (*readCursor) {
        bool splitHere = false;
        bool isTokenChar = func_08050708(*readCursor);
        if (*commentMode == 2) {
            if (processedCharCount > 1 && readCursor[-2] == '*' && readCursor[-1] == '/') {
                *commentMode = 0;
                splitHere = true;
            }
        } else if (*readCursor == '/' && readCursor[1] == '*') {
            *commentMode = 2;
            splitHere = true;
        } else if (*readCursor == '/' && readCursor[1] == '/') {
            if (*commentMode != 3)
                splitHere = true;
            *commentMode = 3;
        } else if (*readCursor == ';') {
            if (*commentMode != 1)
                splitHere = true;
            *commentMode = 1;
        }
        if (*commentMode == 0) {
            if (processedCharCount > 1 && *readCursor != ':' && readCursor[-1] == ':' && readCursor[-2] != ':')
                splitHere = true;
            if (*readCursor == 0 || (processedCharCount > 0 && readCursor[-1] == 0))
                splitHere = true;
        }
        if (*commentMode != 0)
            isTokenChar = true;
        if (splitHere && currentToken) {
            currentToken->separator = 0;
            *writeCursor++ = 0;
            tokenState = 1;
            currentToken = 0;
        }
        switch (tokenState) {
        case 0:
            if (!isTokenChar) {
                if (*readCursor == ',') {
                    if (!previousToken) {
                        reportInfo("unexpected comma encountered\n");
                        return 0;
                    }
                    previousToken->separator = 1;
                    previousToken = 0;
                }
                *writeCursor++ = 0;
                tokenState = 1;
                currentToken = 0;
            } else {
                if (!currentToken) {
                    reportError(locTagNullTeError);
                    return 0;
                }
                *writeCursor++ = *readCursor;
            }
            break;
        case 1:
            if (currentToken) {
                reportError(locTagNonNullTeError);
                return 0;
            }
            if (isTokenChar) {
                if (tokenCount >= 200) {
                    reportError(locTagTooManyTokensError);
                    return 0;
                }
                currentToken = &tokens[tokenCount];
                currentToken->textOffset = writeCursor - lineText;
                currentToken->separator = 0;
                currentToken->inComment = *commentMode != 0;
                previousToken = currentToken;
                *writeCursor++ = *readCursor;
                tokenState = 0;
                tokenCount++;
            } else if (*readCursor == ',') {
                if (!previousToken) {
                    reportInfo("unexpected comma encountered\n");
                    return 0;
                }
                previousToken->separator = 1;
                previousToken = 0;
            }
            break;
        }
        processedCharCount++;
        readCursor++;
    }
    if (currentToken) {
        currentToken->separator = 2;
        *writeCursor++ = 0;
    }
    if (processedCharCount > 1 && readCursor[-2] == '*' && readCursor[-1] == '/')
        *commentMode = 0;
    if (*commentMode != 2)
        *commentMode = 0;
    if (!tokenCount)
        lineText = 0;
    return writeCursor;
}

char *LocTag::tokenTextAt(int tokenIndex, char *separatorOut)
{
    if (!tokenCount)
        return 0;
    if (tokenIndex < 0 || tokenIndex >= tokenCount)
        return 0;
    if (!lineText) {
        reportError(locTagGetTokenNullError);
        return 0;
    }
    if (separatorOut)
        *separatorOut = tokens[tokenIndex].separator;
    return lineText + tokens[tokenIndex].textOffset;
}

void LocTag::dumpSourceLine(int highlightedToken)
{
    if (!isValid()) {
        reportError(locTagInvalidDumpError);
        return;
    }
    if (gaspDepth > 0) {
        for (int d = 0; d != gaspDepth; d++) {
            for (int k = 0; k != d; k++)
                reportMessage("  ");
            if (!gaspStack[d].sourceLocation)
                reportError(locTagDumpNoPtrError, d, gaspDepth);
            reportMessage(" line %4d ", gaspStack[d].gaspLineNumber);
            gaspStack[d].sourceLocation->dumpLocation();
        }
    }
    if (!tokenCount && lineText) {
        reportMessage("  ---> '%s'", lineText);
    } else {
        reportMessage("  ---> ");
        bool wasInComment = false;
        for (int i = 0; i != tokenCount; i++) {
            char *tokenText = tokenTextAt(i, 0);
            if (i == highlightedToken)
                reportMessage("[***");
            if (inComment(i) && !wasInComment)
                reportMessage(" [ ");
            reportMessage("{");
            reportMessage("%s", tokenText);
            reportMessage("}");
            if (i == highlightedToken)
                reportMessage("***]");
            if (!inComment(i) && wasInComment)
                reportMessage(" ] ");
            wasInComment = inComment(i);
            if (tokens[i].separator == 1)
                reportMessage(",");
            else
                reportMessage(" ");
        }
        if (wasInComment)
            reportMessage("] ");
    }
    reportMessage("\n");
}

bool LocTag::startsWithDirective()
{
    char *s = tokenTextAt(0, 0);
    if (s && *s == '.')
        return true;
    return false;
}

bool LocTag::isCommentToken(int i)
{
    if (i >= 0 && i < tokenCount)
        return tokens[i].inComment != 0;
    return true;
}

bool LocTag::hasAssignmentMarker()
{
    char *s = tokenTextAt(0, 0);
    if (s && s[0] == '=' && s[1] == 0)
        return true;
    return false;
}

bool LocTag::func_08050f7c(int i)
{
    if (!inComment(i))
        tokenTextAt(i, 0);
    return false;
}

