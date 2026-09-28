// Source parser (CIE group 2, after xun.cpp).
// xpTree unknown-collection helpers live at the head of this unit (address order).

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);

int g_080b3140 = 0;

int xpTree::func_0804f8f0()
{
    g_080b3140 = 0;
    xpNode *n = root;
    if (n)
        func_0804f920(n);
    return g_080b3140;
}

void xpTree::func_0804f920(xpNode *n)
{
    if (g_080b3140 < 0)
        return;
    if (n->unk_04) {
        for (int i = 0; i != n->unk_04; i++) {
            if (n->child[i]) {
                func_0804f920(n->child[i]);
                if (g_080b3140 < 0)
                    return;
            }
        }
    } else {
        if (n->val.makeNumeric())
            return;
        // Same fallback as xpNode::getLiteral(), open-coded.
        const char *s = n->val.getString();
        if (!s)
            s = "<no_literal>";
        if (s && !func_0804f75c(s)) {
            func_0804ad38("failed to add an unknown\n");
            g_080b3140 = -1;
            return;
        }
        g_080b3140++;
    }
}

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

extern char *g_080b3148;    // whitespace set
extern char *g_080b314c;    // block-start keyword
extern char *g_080b3150;    // block-end keyword

bool func_08050780(const char *s, const char *kw);

static inline bool isEol(char c)
{
    return c == '\n' || c == '\r';
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
        if (*p == '\n')
            nlines++;
    unk_1009c = new LocTag[nlines];
    if (!unk_1009c) {
        func_0804ad38("Parser: failed to malloc line table\n");
        reset();
        return false;
    }
    unk_10098 = 0;
    int lineNo = 0;
    char *out = unk_10094;
    char *line = unk_10090;
    int commentMode = 0;
    while (*line) {
        char *p = line;
        while (*p && *p != '\n')
            p++;
        char *eol = p;
        while (*p && isEol(*p))
            p++;
        char *next = p;
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
        line = next;
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
