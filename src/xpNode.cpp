#include <stdio.h>
#include "xpNode.h"
#include "os.h"

xpNode::xpNode()
{
    init();
}


xpNode::~xpNode()
{
    val.clear();
}

bool xpNode::func_0804e778()
{
    clearValue();
    return true;
}

void xpNode::func_0804e794()
{
    val.dump();
}

void xpNode::reset()
{
    init();
}

bool xpNode::isFull()
{
    return unk_05 >= unk_04;
}

bool xpNode::decode()
{
    if (unk_02) {
        printf("can't decode a non-literal\n");
        return false;
    }
    if (!val.makeNumeric()) {
        func_0804ad38("couldn't convert to arithmetic\n");
        return false;
    }
    return true;
}

const char *xpNode::getLiteral()
{
    char *s = val.getString();
    if (s)
        return s;
    return "<no_literal>";
}

xpTree::xpTree()
{
    root = 0;
    unk_00 = 0;
    clear();
}

bool xpTree::dispose(xpNode *n)
{
    if (!n)
        return true;
    for (int i = 0; i != n->unk_05; i++)
        if (!dispose(n->child[i]))
            return false;
    delete n;
    return true;
}

bool xpTree::reset()
{
    if (!dispose(root)) {
        func_0804ad38("failed to dispose xn\n");
        return false;
    }
    unk_08 = 0;
    unk_0c = 0;
    root = 0;
    unk_00 = 0;
    return true;
}

void xpTree::calcDepths()
{
    unk_0c = 0;
    xpNode *n = root;
    if (n) {
        n->unk_01 = 0;
        for (int i = 0; i != n->unk_05; i++)
            setDepth(n->child[i], 1);
    }
}
