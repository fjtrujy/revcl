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
