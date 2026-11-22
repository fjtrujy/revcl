#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool asciiCaseEqual(const char *a, const char *b);
bool func_08050780(const char *s, const char *kw);

char *g_080b3144 = "0123456789.";    // gasp line-number chars
char *g_080b3148 = "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?;";    // token (non-whitespace) char set
char *g_080b314c = "----*";    // block-start keyword
char *g_080b3150 = "*----";    // block-end keyword

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
    int len = mstrlen(buf);
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
    mstrcpy(rawBuffer, buf);
    if (addLocationTag(name, 1)) {
        reportError("Parser::failed to add initial loctag!\n");
        return false;
    }
    gaspLocationDepth = 1;
    gaspLocationStack[0].unk_04 = 0;
    gaspLocationStack[0].unk_00 = lines;
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
        lines[locationTagCount].func_0805109c();
    }
}

static inline bool isEol(char c)
{
    return c == '\n' || c == '\r';
}

static inline bool isNewline(char c)
{
    return c == '\n';
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
    int len = mstrlen(rawBuffer);
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
    LocTag *t;
    int lineNo = 0;
    char *next;
    char *line = rawBuffer;
    int commentMode = 0;
    char *out = processedBuffer;
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
        if ((char)checkBlock && func_08050780(line, g_080b314c)) {
            char *q = next;
            t = &parsedLines[parsedLineCount++];
            t->unk_04 = lineNo;
            char rawHead = *next;
            for (;;) {
                if (!rawHead)
                    break;
                if (func_08050780(q, g_080b3150)) {
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
            t->unk_0a = -1;
            t->unk_00 = next;
            t->unk_08 = 1;
            next = q;
            process = false;
        }
        if (process) {
            char *s = line;
            char c;
            while ((c = *s) && !({
                const char *q = g_080b3148;
                char tok;
                while ((tok = *q) && tok != c)
                    q++;
                tok != 0;
            }))
                s++;
            t = &parsedLines[parsedLineCount];
            if (gasp) {
                t->unk_0a = gaspLocationDepth;
                for (int i = 0; i != gaspLocationDepth; i++)
                    t->unk_0c[i] = gaspLocationStack[i];
            } else {
                t->unk_0a = -1;
            }
            t->unk_04 = lineNo;
            parsedLineCount++;
            t->unk_00 = out;
            out = t->tokenizeSourceLine(&commentMode, line);
            if (!out) {
                reportError("Parser: failed to tokenise line '%s'\n", line);
                dumpLineEntry(t, -1);
                return false;
            }
        }
        lineNo++;
    }
    return true;
}

static inline bool isWhite(const char *s)
{
    const char *q = g_080b3148;
    while (*q && *q != *s)
        q++;
    return *q != 0;
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

bool func_080501c0(const char *p)
{
    return *p == '\n';
}

bool func_080501d4(char c)
{
    return c == '\n' || c == '\r';
}

void Parser::Dump()
{
    if (!parsedLines) {
        reportError("Parse::Dump() - line table was not set\n");
        return;
    }
    for (int i = 0; i != parsedLineCount; i++)
        parsedLines[i].dumpSourceLine(-1);
    reportMessage("parser::DumpLocTags..\n");
    for (int i = 0; i != locationTagCount; i++) {
        reportMessage("%4d : ", i);
        lines[i].func_080510cc();
        reportMessage("\n");
    }
}

void Parser::DumpLocTags()
{
    reportMessage("parser::DumpLocTags..\n");
    for (int i = 0; i != locationTagCount; i++) {
        reportMessage("%4d : ", i);
        lines[i].func_080510cc();
        reportMessage("\n");
    }
}

int Parser::addLocationTag(const char *name, int n)
{
    int idx;
    for (int i = 0; i < locationTagCount; i++) {
        if (lines[i].unk_04 == n && func_0804c03c(lines[i].unk_00, name)) {
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
        if (!l->func_08051054(name, n)) {
            reportError("failed to set up a loc tag!\n");
            return 0;
        }
        idx = locationTagCount++;
    }
    return idx;
}

int Parser::FindLocTag(const char *name, int n)
{
    for (int i = 0; i < locationTagCount; i++)
        if (lines[i].unk_04 == n && func_0804c03c(lines[i].unk_00, name))
            return i;
    return -1;
}

static inline bool isGaspDigit(unsigned char c)
{
    for (const char *q = g_080b3144; *q; q++)
        if ((unsigned char)*q == c)
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
            gaspLocationStack[depth - 1].unk_00 = &lines[idx];
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
        gaspLocationStack[i].unk_04 = stack[i];
    return true;
}

bool func_080506e0(const char *s)
{
    if (!s || !*s)
        return false;
    while (*s)
        s++;
    return s[-1] == ':';
}

bool func_08050708(char c)
{
    const char *q = g_080b3148;
    char ch;
    while ((ch = *q) && ch != c)
        q++;
    return *q != 0;
}

bool Parser::isAssignmentLine(int i)
{
    LocTag *t;
    if (i < 0 || i >= parsedLineCount)
        t = 0;
    else
        t = &parsedLines[i];
    if (t)
        return t->func_08050f48();
    return false;
}

bool func_08050780(const char *s, const char *kw)
{
    if (s) {
        while (*s && !isWhite(s) && *s != '\n')
            s++;
        while (*kw && *s && *s == *kw) {
            s++;
            kw++;
        }
        if (!*kw)
            return true;
    }
    return false;
}

bool func_080507f4(const char *s)
{
    return func_08050780(s, g_080b314c);
}

bool func_08050810(const char *s)
{
    return func_08050780(s, g_080b3150);
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

