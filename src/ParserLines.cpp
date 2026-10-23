// Remaining parser methods and token helpers.

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


extern const char parserDumpNoTable[];
extern const char parserLocTagLimit[];
extern const char parserGaspIncludeTagFailure[];
extern const char parserGaspMissingPreviousTag[];
extern const char parserGaspDepthJump[];
extern const char parserGaspDepthOverflow[];

extern char *g_080b3144;
extern char *g_080b3148;
extern char *g_080b314c;
extern char *g_080b3150;

bool func_08050780(const char *s, const char *kw);

static inline bool isWhite(const char *s)
{
    const char *q = g_080b3148;
    while (*q && *q != *s)
        q++;
    return *q != 0;
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
        func_0804ad38(parserDumpNoTable);
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
            func_0804ad38(parserLocTagLimit);
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
            func_0804ad38(parserGaspDepthOverflow);
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
            func_0804ad38(parserGaspDepthJump);
            return false;
        }
        if (d == 1) {
            int idx;
            if (unk_10008)
                idx = func_0805030c(unk_10008, 2);
            else if (unk_10004)
                idx = func_0805030c(unk_10004, 1);
            else {
                func_0804ad38(parserGaspMissingPreviousTag);
                return false;
            }
            if (idx < 0) {
                func_0804ad38(parserGaspIncludeTagFailure);
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
    const char *q = g_080b3148;
    char ch;
    while ((ch = *q) && ch != c)
        q++;
    return *q != 0;
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

bool __attribute__((weak,section(".gnu.linkonce.t._ZN6Parser13func_0804fb94EPcPKcbb"))) Parser::func_0804fb94(char *buf, const char *name, bool gasp, bool cpp)
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

// Source parser (CIE group 2, after xun.cpp).
// xpTree unknown-collection helpers live at the head of this unit (address order).

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);

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
// Parser lifetime and unknown-list cleanup; user-authorized compiler-state reconstruction.

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

void func_0804f6ec()
{
    while (g_080bcda4) {
        xun *p = g_080bcda4;
        g_080bcda4 = p->next;
        delete p;
    }
    g_080bcda4 = 0;
    g_080bcda8 = 0;
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

// Unknown-name list (CIE group 2, after xpNode.cpp).

#include "xun.h"
#include "os.h"
#include "xpNode.h"

bool func_0804bfcc(const char *a, const char *b);
char *dupString(const char *s);

xun *g_080bcda4;
int g_080bcda8;

bool func_0804f75c(const char *s)
{
    if (!s) {
        func_0804ad38("passed a null string to add unknown\n");
        return false;
    }
    for (xun *p = g_080bcda4; p; p = p->next)
        if (func_0804bfcc(p->name, s))
            return true;
    xun *x = new xun;
    if (!x) {
        func_0804ad38("failed to new xun\n");
        return false;
    }
    x->name = dupString(s);
    if (!x->name) {
        func_0804ad38("failed to dup string for xun\n");
        delete x;
        return false;
    }
    g_080bcda8++;
    x->next = g_080bcda4;
    g_080bcda4 = x;
    return true;
}

void func_0804f844()
{
    for (xun *p = g_080bcda4; p; p = p->next)
        ;
}
