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
    unk_10010 = 0;
    unk_1000c = lines;
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
