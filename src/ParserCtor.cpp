// Parser lifetime and unknown-list cleanup; user-authorized compiler-state reconstruction.

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


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
