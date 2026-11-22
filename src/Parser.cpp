#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int checkedStringLength(const char *s);
bool equalExactStrings(const char *a, const char *b);
bool asciiCaseEqual(const char *a, const char *b);
bool matchesLeadingKeyword(const char *s, const char *kw);

char *gaspLineNumberCharacters = "0123456789.";    // gasp line-number chars
char *tokenizerCharacterSet = "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?;";    // token (non-whitespace) char set
char *blockStartKeyword = "----*";    // block-start keyword
char *blockEndKeyword = "*----";    // block-end keyword

Parser::Parser()
{
    rawBuffer = 0;
    processedBuffer = 0;
    parsedLines = 0;
    parsedLineCount = 0;
    locationTagCount = 0;
}

Parser::~Parser()
{
    reset();
}

bool Parser::initializeParser(char *buf, const char *name, bool gasp, bool cpp)
{
    reset();
    if (!buf) {
        reportError("Parser::Init: passed a null pointer to buffer\n");
        reset();
        return false;
    }
    int len = checkedStringLength(buf);
    if (!len) {
        reportError("Parser:: was passed a null string!\n");
        reset();
        return false;
    }
    rawBuffer = new char[len + 16];
    if (!rawBuffer) {
        reportError("Parser:: malloc failed for raw buffer\n");
        reset();
        return false;
    }
    checkedStringCopy(rawBuffer, buf);
    if (addLocationTag(name, 1)) {
        reportError("Parser::failed to add initial loctag!\n");
        return false;
    }
    gaspLocationDepth = 1;
    gaspLocationStack[0].gaspLineNumber = 0;
    gaspLocationStack[0].sourceLocation = lines;
    if (!buildProcessedLines(gasp, cpp)) {
        reportError("Parser: failed to build proc buffer\n");
        reset();
        return false;
    }
    return true;
}

void Parser::reset()
{
    if (rawBuffer) {
        delete[] rawBuffer;
        rawBuffer = 0;
    }
    if (processedBuffer) {
        delete[] processedBuffer;
        processedBuffer = 0;
    }
    if (parsedLines) {
        delete[] parsedLines;
        parsedLines = 0;
    }
    parsedLineCount = 0;
    while (locationTagCount) {
        locationTagCount--;
        lines[locationTagCount].clearLocation();
    }
}

static inline bool isEol(char character)
{
    return character == '\n' || character == '\r';
}

static inline bool isNewline(char character)
{
    return character == '\n';
}

static inline void dumpLineEntry(LocTag *entry, int highlight)
{
    entry->dumpSourceLine(highlight);
}

bool Parser::buildProcessedLines(bool gasp, bool cpp)
{
    if (!rawBuffer) {
        reportError("Parser: build proc buffer - raw buffer was null pointer\n");
        reset();
        return false;
    }
    int len = checkedStringLength(rawBuffer);
    if (!len) {
        reportError("arser: raw buffer was null string\n");
        reset();
        return false;
    }
    processedBuffer = new char[len * 2];
    if (!processedBuffer) {
        reportError("Parser: malloc failed for proc buffer\n");
        reset();
        return false;
    }
    int nlines = 0;
    char ch;
    for (char *p = rawBuffer; (ch = *p); p++)
        if (isNewline(ch))
            nlines++;
    parsedLines = new LocTag[nlines];
    if (!parsedLines) {
        reportError("Parser: failed to malloc line table\n");
        reset();
        return false;
    }
    parsedLineCount = 0;
    LocTag *locationTag;
    int lineNo = 0;
    char *next;
    char *line = rawBuffer;
    int commentMode = 0;
    char *outputBuffer = processedBuffer;
    for (; *line; line = next) {
        char *p = line;
        while (*p && *p != '\n')
            p++;
        char *eol = p;
        while (*p && isEol(*p))
            p++;
        unsigned char z = (unsigned char)*p;
        z >>= 4;
        z >>= 4;
        next = p + z;
        *eol = 0;
        bool process = true;
        if (*line == ';' && gasp) {
            if (!decodeGaspLine(line)) {
                reportError("failed to decode a gasp line\n");
                return false;
            }
            process = false;
        }
        int checkBlock = process;
        if ((char)checkBlock && matchesLeadingKeyword(line, blockStartKeyword)) {
            char *q = next;
            locationTag = &parsedLines[parsedLineCount++];
            locationTag->sourceLineNumber = lineNo;
            char rawHead = *next;
            for (;;) {
                if (!rawHead)
                    break;
                if (matchesLeadingKeyword(q, blockEndKeyword)) {
                    rawHead = *q;
                    break;
                }
                rawHead = *q;
                while (rawHead && !isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                while (rawHead && isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                lineNo++;
            }
            char *e = q;
            if (rawHead) {
                while (rawHead && !isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                while (rawHead && isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                lineNo++;
            }
            *e = 0;
            locationTag->gaspDepth = -1;
            locationTag->lineText = next;
            locationTag->isRawMacroBlock = 1;
            next = q;
            process = false;
        }
        if (process) {
            char *s = line;
            char c;
            while ((c = *s) && !({
                const char *q = tokenizerCharacterSet;
                char tok;
                while ((tok = *q) && tok != c)
                    q++;
                tok != 0;
            }))
                s++;
            locationTag = &parsedLines[parsedLineCount];
            if (gasp) {
                locationTag->gaspDepth = gaspLocationDepth;
                for (int i = 0; i != gaspLocationDepth; i++)
                    locationTag->gaspStack[i] = gaspLocationStack[i];
            } else {
                locationTag->gaspDepth = -1;
            }
            locationTag->sourceLineNumber = lineNo;
            parsedLineCount++;
            locationTag->lineText = outputBuffer;
            outputBuffer = locationTag->tokenizeSourceLine(&commentMode, line);
            if (!outputBuffer) {
                reportError("Parser: failed to tokenise line '%s'\n", line);
                dumpLineEntry(locationTag, -1);
                return false;
            }
        }
        lineNo++;
    }
    return true;
}

static inline bool isWhite(const char *text)
{
    const char *allowedCharacter = tokenizerCharacterSet;
    while (*allowedCharacter && *allowedCharacter != *text)
        allowedCharacter++;
    return *allowedCharacter != 0;
}

int Parser::getParsedLineCount()
{
    return parsedLineCount;
}

LocTag *Parser::getParsedLine(int i)
{
    if (i < 0 || i >= parsedLineCount)
        return 0;
    return &parsedLines[i];
}

bool isLineFeedAt(const char *text)
{
    return *text == '\n';
}

bool isLineEndingCharacter(char character)
{
    return character == '\n' || character == '\r';
}

void Parser::dumpParsedSourceLines()
{
    if (!parsedLines) {
        reportError("Parse::Dump() - line table was not set\n");
        return;
    }
    for (int lineIndex = 0; lineIndex != parsedLineCount; lineIndex++)
        parsedLines[lineIndex].dumpSourceLine(-1);
    reportMessage("parser::DumpLocTags..\n");
    for (int tagIndex = 0; tagIndex != locationTagCount; tagIndex++) {
        reportMessage("%4d : ", tagIndex);
        lines[tagIndex].dumpLocation();
        reportMessage("\n");
    }
}

void Parser::dumpLocationTags()
{
    reportMessage("parser::DumpLocTags..\n");
    for (int lineIndex = 0; lineIndex != locationTagCount; lineIndex++) {
        reportMessage("%4d : ", lineIndex);
        lines[lineIndex].dumpLocation();
        reportMessage("\n");
    }
}

int Parser::addLocationTag(const char *name, int n)
{
    int idx;
    for (int i = 0; i < locationTagCount; i++) {
        if (lines[i].locationKind == n && equalExactStrings(lines[i].locationName, name)) {
            idx = i;
            goto found;
        }
    }
    idx = -1;
found:
    if (idx < 0) {
        if (locationTagCount >= 0x2000) {
            reportError("AddLocTag: ran out of loctags\n");
            return -1;
        }
        Line *l = &lines[locationTagCount];
        if (!l->setLocation(name, n)) {
            reportError("failed to set up a loc tag!\n");
            return 0;
        }
        idx = locationTagCount++;
    }
    return idx;
}

int Parser::findLocationTagByName(const char *name, int locationKind)
{
    for (int lineIndex = 0; lineIndex < locationTagCount; lineIndex++)
        if (lines[lineIndex].locationKind == locationKind && equalExactStrings(lines[lineIndex].locationName, name))
            return lineIndex;
    return -1;
}

static inline bool isGaspDigit(unsigned char character)
{
    for (const char *allowedCharacter = gaspLineNumberCharacters; *allowedCharacter; allowedCharacter++)
        if ((unsigned char)*allowedCharacter == character)
            return true;
    return false;
}

bool Parser::decodeGaspLine(char *line)
{
    int stack[16];
    int depth = 0;
    char *p = line;
    if (!p)
        return false;
    if (*p != ';')
        return false;
    p++;
    while (*p && !isGaspDigit(*p))
        p++;
    while (isGaspDigit(*p)) {
        int n = parseInteger(p);
        if (!n) {
            reportError("bad gasp char in line %s\n", line);
            return false;
        }
        if (depth > 15) {
            reportError("exceeded maximum depth for the loctag stack\n");
            return false;
        }
        stack[depth++] = n;
        while (isGaspDigit(*p) && *p != '.')
            p++;
        if (*p != '.')
            break;
        p++;
    }
    if (!depth) {
        reportError("no gasp line entries!\n");
        return false;
    }
    if (depth != gaspLocationDepth) {
        int d = depth - gaspLocationDepth;
        if (d > 1) {
            reportError("jumped too far  in gasp depth\n");
            return false;
        }
        if (d == 1) {
            int idx;
            if (pendingMacroName)
                idx = addLocationTag(pendingMacroName, 2);
            else if (pendingIncludeName)
                idx = addLocationTag(pendingIncludeName, 1);
            else {
                reportError("neither previous include or previous macro set\n");
                return false;
            }
            if (idx < 0) {
                reportError("failed to add loc tag for include directive\n");
                return false;
            }
            gaspLocationStack[depth - 1].sourceLocation = &lines[idx];
        }
    }
    int isInclude = 0;
    for (;;) {
        while (*p && !isWhite(p))
            p++;
        if (!*p)
            break;
        char *e = p;
        while (*e && isWhite(e))
            e++;
        *e = 0;
        if (asciiCaseEqual(p, ".include")) {
            p = e + 1;
            isInclude = 1;
            continue;
        }
        if (e[-1] == ':') {
            p = e;
            continue;
        }
        if (isInclude) {
            pendingIncludeName = p;
            pendingMacroName = 0;
        } else {
            pendingMacroName = p;
            pendingIncludeName = 0;
        }
        break;
    }
    gaspLocationDepth = depth;
    for (int i = 0; i != gaspLocationDepth; i++)
        gaspLocationStack[i].gaspLineNumber = stack[i];
    return true;
}

bool hasTrailingLabelColon(const char *text)
{
    if (!text || !*text)
        return false;
    while (*text)
        text++;
    return text[-1] == ':';
}

bool isTokenSeparatorCharacter(char character)
{
    const char *separatorCursor = tokenizerCharacterSet;
    char separatorCharacter;
    while ((separatorCharacter = *separatorCursor) && separatorCharacter != character)
        separatorCursor++;
    return *separatorCursor != 0;
}

bool Parser::isAssignmentLine(int i)
{
    LocTag *locationTag;
    if (i < 0 || i >= parsedLineCount)
        locationTag = 0;
    else
        locationTag = &parsedLines[i];
    if (locationTag)
        return locationTag->hasAssignmentMarker();
    return false;
}

bool matchesLeadingKeyword(const char *text, const char *keyword)
{
    if (text) {
        while (*text && !isWhite(text) && *text != '\n')
            text++;
        while (*keyword && *text && *text == *keyword) {
            text++;
            keyword++;
        }
        if (!*keyword)
            return true;
    }
    return false;
}

bool matchesFirstSpecialKeyword(const char *text)
{
    return matchesLeadingKeyword(text, blockStartKeyword);
}

bool matchesSecondSpecialKeyword(const char *text)
{
    return matchesLeadingKeyword(text, blockEndKeyword);
}

extern const char locTagInvalidCommentError[] = "invalid comment mode at start of lineEntry::Tokenise\n";
extern const char locTagTooManyTokensError[] = "lineEntry::Tokenise: too many tokens on line\n";
extern const char locTagNonNullTeError[] = "tokeniser had unexpected non-null te\n";
extern const char locTagNullTeError[] = "tokeniser had unexpected null TE\n";
extern const char locTagNullInPError[] = "lineEntry::Tokenise() - in_p was null\n";
extern const char locTagNullLineDataError[] = "lineEntry::Tokenise() - m_l_LineData was null\n";
extern const char locTagGetTokenNullError[] = "lineEntry: get token - but m_lineData was null\n";
extern const char locTagDumpNoPtrError[] = "lineEntry::Dump() - had no locTag pointer set at depth %d/%d\n";
extern const char locTagInvalidDumpError[] = "lineEntry() was invalid - couldn't dump\n";

