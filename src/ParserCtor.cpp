// Source parser (CIE group 2, after xun.cpp).
// xpTree unknown-collection helpers live at the head of this unit (address order).

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


Parser::Parser()
{
    unk_10090 = 0;
    unk_10094 = 0;
    unk_1009c = 0;
    unk_10098 = 0;
    unk_10000 = 0;
}

Parser::~Parser()
{
    reset();
}

bool Parser::func_0804fb94(char *buf, const char *name, bool gasp, bool cpp)
{
    reset();
    if (!buf) {
        func_0804ad38("Parser::Init: passed a null pointer to buffer\n");
        reset();
        return false;
    }
    int len = mstrlen(buf);
    if (!len) {
        func_0804ad38("Parser:: was passed a null string!\n");
        reset();
        return false;
    }
    unk_10090 = new char[len + 16];
    if (!unk_10090) {
        func_0804ad38("Parser:: malloc failed for raw buffer\n");
        reset();
        return false;
    }
    mstrcpy(unk_10090, buf);
    if (func_0805030c(name, 1)) {
        func_0804ad38("Parser::failed to add initial loctag!\n");
        return false;
    }
    unk_1008c = 1;
    unk_1000c[0].unk_04 = 0;
    unk_1000c[0].unk_00 = lines;
    if (!func_0804fd80(gasp, cpp)) {
        func_0804ad38("Parser: failed to build proc buffer\n");
        reset();
        return false;
    }
    return true;
}

void Parser::reset()
{
    if (unk_10090) {
        delete[] unk_10090;
        unk_10090 = 0;
    }
    if (unk_10094) {
        delete[] unk_10094;
        unk_10094 = 0;
    }
    if (unk_1009c) {
        delete[] unk_1009c;
        unk_1009c = 0;
    }
    unk_10098 = 0;
    while (unk_10000) {
        unk_10000--;
        lines[unk_10000].func_0805109c();
    }
}

char *g_080b3144 = "0123456789.";    // gasp line-number chars
char *g_080b3148 = "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?;";    // token (non-whitespace) char set
char *g_080b314c = "----*";    // block-start keyword
char *g_080b3150 = "*----";    // block-end keyword

bool func_08050780(const char *s, const char *kw);

static inline bool isEol(char c)
{
    return c == '\n' || c == '\r';
}

static inline bool isNewline(const char *p)
{
    return *p == '\n';
}

static inline bool isWhite(const char *s)
{
    const char *q = g_080b3148;
    while (*q && *q != *s)
        q++;
    return *q != 0;
}

bool Parser::func_0804fd80(bool gasp, bool cpp)
{
    if (!unk_10090) {
        func_0804ad38("Parser: build proc buffer - raw buffer was null pointer\n");
        reset();
        return false;
    }
    int len = mstrlen(unk_10090);
    if (!len) {
        func_0804ad38("arser: raw buffer was null string\n");
        reset();
        return false;
    }
    unk_10094 = new char[len * 2];
    if (!unk_10094) {
        func_0804ad38("Parser: malloc failed for proc buffer\n");
        reset();
        return false;
    }
    int nlines = 0;
    for (char *p = unk_10090; *p; p++)
        if (isNewline(p))
            nlines++;
    unk_1009c = new LocTag[nlines];
    if (!unk_1009c) {
        func_0804ad38("Parser: failed to malloc line table\n");
        reset();
        return false;
    }
    unk_10098 = 0;
    char *next;
    int lineNo = 0;
    char *out = unk_10094;
    int commentMode = 0;
    for (char *line = unk_10090; *line; line = next) {
        char *p = line;
        while (*p && *p != '\n')
            p++;
        char *eol = p;
        while (*p && isEol(*p))
            p++;
        next = p;
        *eol = 0;
        bool process = true;
        if (*line == ';' && gasp) {
            if (!func_0805041c(line)) {
                func_0804ad38("failed to decode a gasp line\n");
                return false;
            }
            process = false;
        }
        if (process && func_08050780(line, g_080b314c)) {
            LocTag *t = &unk_1009c[unk_10098++];
            t->unk_04 = lineNo;
            char *q = next;
            while (*q) {
                if (func_08050780(q, g_080b3150))
                    break;
                while (*q && !isEol(*q))
                    q++;
                while (*q && isEol(*q))
                    q++;
                lineNo++;
            }
            char *e = q;
            if (*q) {
                while (*q && !isEol(*q))
                    q++;
                while (*q && isEol(*q))
                    q++;
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
            while (*s && isWhite(s))
                s++;
            LocTag *t = &unk_1009c[unk_10098];
            if (gasp) {
                t->unk_0a = unk_1008c;
                for (int i = 0; i != unk_1008c; i++)
                    t->unk_0c[i] = unk_1000c[i];
            } else {
                t->unk_0a = -1;
            }
            t->unk_04 = lineNo;
            unk_10098++;
            t->unk_00 = out;
            out = t->func_0805086c(&commentMode, line);
            if (!out) {
                func_0804ad38("Parser: failed to tokenise line '%s'\n", line);
                t->func_08050c40(-1);
                return false;
            }
        }
        lineNo++;
    }
    return true;
}

int Parser::func_0805017c()
{
    return unk_10098;
}

LocTag *Parser::func_0805018c(int i)
{
    if (i < 0 || i >= unk_10098)
        return 0;
    return &unk_1009c[i];
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
    if (!unk_1009c) {
        func_0804ad38("Parse::Dump() - line table was not set\n");
        return;
    }
    for (int i = 0; i != unk_10098; i++)
        unk_1009c[i].func_08050c40(-1);
    func_0804ac50("parser::DumpLocTags..\n");
    for (int i = 0; i != unk_10000; i++) {
        func_0804ac50("%4d : ", i);
        lines[i].func_080510cc();
        func_0804ac50("\n");
    }
}

void Parser::DumpLocTags()
{
    func_0804ac50("parser::DumpLocTags..\n");
    for (int i = 0; i != unk_10000; i++) {
        func_0804ac50("%4d : ", i);
        lines[i].func_080510cc();
        func_0804ac50("\n");
    }
}

int Parser::func_0805030c(const char *name, int n)
{
    int idx;
    for (int i = 0; i < unk_10000; i++) {
        if (lines[i].unk_04 == n && func_0804c03c(lines[i].unk_00, name)) {
            idx = i;
            goto found;
        }
    }
    idx = -1;
found:
    if (idx < 0) {
        if (unk_10000 >= 0x2000) {
            func_0804ad38("AddLocTag: ran out of loctags\n");
            return -1;
        }
        Line *l = &lines[unk_10000];
        if (!l->func_08051054(name, n)) {
            func_0804ad38("failed to set up a loc tag!\n");
            return 0;
        }
        idx = unk_10000++;
    }
    return idx;
}

int Parser::FindLocTag(const char *name, int n)
{
    for (int i = 0; i < unk_10000; i++)
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

bool Parser::func_0805041c(char *line)
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
        int n = func_0804add4(p);
        if (!n) {
            func_0804ad38("bad gasp char in line %s\n", line);
            return false;
        }
        if (depth > 15) {
            func_0804ad38("exceeded maximum depth for the loctag stack\n");
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
        func_0804ad38("no gasp line entries!\n");
        return false;
    }
    if (depth != unk_1008c) {
        int d = depth - unk_1008c;
        if (d > 1) {
            func_0804ad38("jumped too far  in gasp depth\n");
            return false;
        }
        if (d == 1) {
            int idx;
            if (unk_10008)
                idx = func_0805030c(unk_10008, 2);
            else if (unk_10004)
                idx = func_0805030c(unk_10004, 1);
            else {
                func_0804ad38("neither previous include or previous macro set\n");
                return false;
            }
            if (idx < 0) {
                func_0804ad38("failed to add loc tag for include directive\n");
                return false;
            }
            unk_1000c[depth - 1].unk_00 = &lines[idx];
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
        if (func_0804bfcc(p, ".include")) {
            p = e + 1;
            isInclude = 1;
            continue;
        }
        if (e[-1] == ':') {
            p = e;
            continue;
        }
        if (isInclude) {
            unk_10004 = p;
            unk_10008 = 0;
        } else {
            unk_10008 = p;
            unk_10004 = 0;
        }
        break;
    }
    unk_1008c = depth;
    for (int i = 0; i != unk_1008c; i++)
        unk_1000c[i].unk_04 = stack[i];
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
    return isWhite(&c);
}

bool Parser::func_08050738(int i)
{
    LocTag *t;
    if (i < 0 || i >= unk_10098)
        t = 0;
    else
        t = &unk_1009c[i];
    if (t)
        return t->func_08050f48();
    return false;
}

bool func_08050780(const char *s, const char *kw)
{
    if (!s)
        return false;
    while (*s && !isWhite(s) && *s != '\n')
        s++;
    while (*kw && *s && *s == *kw) {
        s++;
        kw++;
    }
    if (!*kw)
        return true;
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
