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
    cur = 0;
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
    cur = 0;
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

inline void xpTree::setDepth(xpNode *n, int depth)
{
    if (n) {
        n->unk_01 = depth;
        for (int i = 0; i != n->unk_05; i++)
            setDepth(n->child[i], depth + 1);
    }
}

void xpTree::func_0804ea8c(xpNode *n)
{
    if (n && n->unk_04)
        for (int i = 0; i != n->unk_04; i++)
            func_0804ea8c(n->child[i]);
}

void xpTree::func_0804ead0()
{
    unk_0c = 0;
    xpNode *n = root;
    if (n) {
        n->unk_01 = 0;
        for (int i = 0; i != n->unk_05; i++)
            setDepth(n->child[i], 1);
    }
    func_0804ea8c(root);
}

void xpTree::dumpNode(xpNode *n)
{
    if (!n)
        return;
    for (int i = 0; i != n->unk_01; i++)
        ;
    n->val.dump();
    for (int i = 0; i != n->unk_04; i++) {
        if (n->child[i])
            dumpNode(n->child[i]);
        else
            func_0804ad38("null arg\n");
    }
}

void xpTree::dump()
{
    unk_0c = 0;
    xpNode *n = root;
    if (n) {
        n->unk_01 = 0;
        for (int i = 0; i != n->unk_05; i++)
            setDepth(n->child[i], 1);
    }
    dumpNode(root);
}

const char *func_0804ec74(const char *s, xpNode *node, bool binary)
{
    if (!s)
        return 0;
    if (!node)
        return 0;
    if (!*s)
        return 0;
    int len = 0;
    const char *p = s;
    xpOp *found = 0;
    int state = 1;
    int dots = 0;
    char c = *s;
    do {
        if (c == '(' || c == ')')
            break;
        switch (state) {
        case 1:
            if (c == '.')
                state = dots++ ? 0 : 1;
            else if (c == 'e')
                state = len ? 2 : 0;
            else if (!(c >= '0' && c <= '9') && c != '_')
                state = 0;
            break;
        case 2:
            if (c == '-' || c == '+')
                state = 3;
            else
                state = 0;
            break;
        case 3:
        default:
            state = 0;
        }
        for (int i = 2; i != 20; i++) {
            xpOp *e = &opTable_080b30a0[i];
            if ((len || (binary ? e->arity > 1 : e->arity <= 1)) && state != 3) {
                const char *a = e->name, *b = p;
                for (; *a; a++, b++)
                    if (*b != *a)
                        break;
                if (!*a) {
                    found = e;
                    break;
                }
            }
        }
        if (found)
            break;
        len++;
        p++;
    } while ((c = *p) != 0);
    if (len) {
        found = &opTable_080b30a0[0];
    } else {
        if (!found)
            return 0;
        s = found->name;
        for (const char *q = s; *q; q++, p++)
            len++;
    }
    if (!node->val.setString(s, len))
        return 0;
    node->unk_02 = found->type;
    node->unk_03 = found->prec;
    node->unk_04 = found->arity;
    return p;
}

bool xpTree::evalExp(const char *s)
{
    const char *p;

    if (!s) {
        puts("evalExp - null string");
        return false;
    }
    if (!clear()) {
        puts("failed to dispose");
        return false;
    }
    root = new xpNode;
    if (!root) {
        puts("couldn't get rootnode");
        return false;
    }
    xpOp *op = 0;
    for (int i = 0; i != 20; i++) {
        if (opTable_080b30a0[i].type == 1) {
            op = &opTable_080b30a0[i];
            break;
        }
    }
    if (!op) {
        puts("no xor for equality");
        goto fail;
    }
    root->unk_04 = op->arity;
    root->unk_03 = op->prec;
    root->unk_02 = op->type;
    if (!root->val.setString("=")) {
        func_0804ad38("not set equ op\n");
        return false;
    }
    cur = root;
    p = s;
    while (*p) {
        if (*p == '(') {
            if (cur->unk_00 == -1) {
                func_0804ad38("too many open brackets!\n");
                goto fail;
            }
            cur->unk_00++;
            p++;
        } else if (*p == ')') {
            if (cur->unk_05 < cur->unk_04) {
                func_0804ad38("unexpected close bracket\n");
                return false;
            }
            while (cur && cur->unk_00 == 0)
                cur = cur->parent;
            if (!cur) {
                func_0804ad38("unbalanced brackets\n");
                return false;
            }
            cur->unk_00--;
            p++;
        } else {
            xpNode *n = new xpNode;
            if (!n) {
                func_0804ad38("failed to get new xn .. bah\n");
                goto fail;
            }
            p = func_0804ec74(p, n, cur->unk_05 >= cur->unk_04);
            if (!p) {
                delete n;
                goto fail;
            }
            if (cur->unk_05 >= cur->unk_04) {
                if (n->unk_04 <= 1) {
                    func_0804ad38("evalExp: syntax error at : nArgs=%d\n", n->unk_04);
                    n->val.dump();
                    delete n;
                    goto fail;
                }
                while (cur && n->unk_03 <= cur->unk_03 && cur->unk_00 == 0)
                    cur = cur->parent;
                if (!cur || cur->unk_05 == 0) {
                    delete n;
                    goto fail;
                }
                xpNode *last = cur->child[cur->unk_05 - 1];
                if (last->parent != cur) {
                    delete n;
                    goto fail;
                }
                n->parent = cur;
                n->child[0] = last;
                n->unk_05 = 1;
                last->parent = n;
                cur->child[cur->unk_05 - 1] = n;
                cur = n;
            } else {
                if (n->unk_04 > 1) {
                    delete n;
                    goto fail;
                }
                cur->child[cur->unk_05] = n;
                cur->unk_05++;
                n->parent = cur;
                if (n->unk_04)
                    cur = n;
            }
        }
    }
    while (cur && cur->unk_00 == 0)
        cur = cur->parent;
    if (!cur)
        return true;
fail:
    clear();
    return false;
}
