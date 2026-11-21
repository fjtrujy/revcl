#include <stdio.h>
#include "xpNode.h"
#include "xun.h"
#include "os.h"

struct xpNodeDel {
    char pad[0x14];
    mValue val;
    ~xpNodeDel() { mValue *v = &val; v->clear(); }
};

struct xunDel {
    xun *next;
    char *name;
    ~xunDel() { delete[] name; name = 0; next = 0; }
};

xpOp opTable_080b30a0[20] = {
    {"LIT", 0, 0, 1},
    {"=", 1, 1, 0},
    {"~", 7, 1, 30},
    {"!", 17, 1, 30},
    {"+", 14, 1, 30},
    {"-", 15, 1, 30},
    {"**", 20, 2, 26},
    {"&&", 11, 2, 15},
    {"||", 14, 2, 14},
    {"*", 4, 2, 25},
    {"/", 5, 2, 25},
    {"%", 6, 2, 25},
    {"#", 18, 1, 30},
    {"+", 2, 2, 20},
    {"-", 3, 2, 20},
    {"<<", 9, 2, 19},
    {">>", 8, 2, 19},
    {"&", 10, 2, 18},
    {"^", 16, 2, 17},
    {"|", 12, 2, 16},
};

inline void xpNode::clearValue()
{
    val.clear();
}

inline void xpNode::init()
{
    unk_01 = 0;
    unk_02 = 0;
    unk_00 = 0;
    unk_04 = 0;
    unk_05 = 0;
    unk_03 = 0;
    val.clear();
    parent = 0;
    child[0] = 0;
    child[1] = 0;
    val.clear();
}

inline bool xpTree::clear()
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

xpNode::xpNode()
{
    init();
}

xpNode::~xpNode()
{
    mValue *v = &val;
    v->clear();
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

static inline bool xpNodeIsFull(xpNode *n) { return n->unk_05 >= n->unk_04; }
static inline bool xpNodeDecode(xpNode *n)
{
    if (n->unk_02) {
        printf("can't decode a non-literal\n");
        return false;
    }
    if (!n->val.makeNumeric()) {
        func_0804ad38("couldn't convert to arithmetic\n");
        return false;
    }
    return true;
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
    delete (xpNodeDel *)n;
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

void xpTree::setDepth(xpNode *n, int depth)
{
    if (n) {
        n->unk_01 = depth;
        const int &d = depth;
        int i = 0;
        if (n->unk_05) {
            int childDepth = d + 1;
            do {
                setDepth(n->child[i], childDepth);
                i++;
            } while (i != n->unk_05);
        }
    }
}

static inline void setDepthInl(xpTree *t, xpNode *n, int depth)
{
    if (n) {
        n->unk_01 = depth;
        const int &d = depth;
        int i = 0;
        if (n->unk_05) {
            int childDepth = d + 1;
            do {
                t->setDepth(n->child[i], childDepth);
                i++;
            } while (i != n->unk_05);
        }
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
            setDepthInl(this, n->child[i], 1);
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
            setDepthInl(this, n->child[i], 1);
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
    do {
        char c = *p;
        if (c == '(' || c == ')')
            break;
        switch (state) {
        case 1:
            if (c == '.') {
                if (dots)
                    state = 0;
                dots++;
            }
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
    } while (*p);
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

static inline xpOp *findOp(unsigned char type)
{
    for (int i = 0; i != 20; i++)
        if (opTable_080b30a0[i].type == type)
            return &opTable_080b30a0[i];
    return 0;
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
    xpOp *op = findOp(1);
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
            p = func_0804ec74(p, n, xpNodeIsFull(cur));
            if (!p) {
                delete (xpNodeDel *)n;
                goto fail;
            }
            if (cur->unk_05 >= cur->unk_04) {
                if (n->unk_04 <= 1) {
                    func_0804ad38("evalExp: syntax error at : nArgs=%d\n", n->unk_04);
                    n->val.dump();
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                while (cur && n->unk_03 <= cur->unk_03 && cur->unk_00 == 0)
                    cur = cur->parent;
                if (!cur || cur->unk_05 == 0) {
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                xpNode *last = cur->child[cur->unk_05 - 1];
                if (last->parent != cur) {
                    delete (xpNodeDel *)n;
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
                    delete (xpNodeDel *)n;
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
    for (; cur; cur = cur->parent)
        if (cur->unk_00 != 0)
            goto fail;
    return true;
fail:
    clear();
    return false;
}

bool xpTree::evaluate(mValue *out, unsigned int type)
{
    out->clear();
    bool ok = false;
    unsigned int u;
    float f;
    int i;
    int r;
    switch (type) {
    case mValue::UINT: {
        u = 0;
        if (!rec_Eval(root))
            r = false;
        else
            r = root->val.getUInt(&u);
        if ((char)r)
            ok = out->setUInt(u);
        break;
    }
    case mValue::FLOAT: {
        if (!rec_Eval(root))
            r = false;
        else
            r = root->val.getFloat(&f);
        if ((char)r)
            ok = out->setFloat(f);
        break;
    }
    case mValue::INT: {
        if (!rec_Eval(root))
            r = false;
        else
            r = root->val.getInt(&i);
        if ((char)r)
            ok = out->setInt(i);
        break;
    }
    case mValue::STRING:
        func_0804ad38("asked to evaluate a tree as a literal!\n");
        func_0804ad38("code this later\n");
        break;
    }
    return ok;
}

bool xpTree::evalUInt(unsigned int *out)
{
    if (!rec_Eval(root))
        return false;
    return root->val.getUInt(out);
}

bool xpTree::evalFloat(float *out)
{
    if (!rec_Eval(root))
        return false;
    return root->val.getFloat(out);
}

bool xpTree::evalInt(int *out)
{
    if (!rec_Eval(root))
        return false;
    return root->val.getInt(out);
}

bool xpTree::rec_Eval(xpNode *n)
{
    if (!n)
        return false;
    mValue *v = &n->val;
    if (!v->isValid()) {
        func_0804ad38("rec_Eval::invalid xpValue\n");
        return false;
    }
    int i;
    for (i = 0; i != n->unk_04; i++)
        if (!rec_Eval(n->child[i]))
            return false;
    if (n->unk_05 != n->unk_04) {
        printf("tree is incomplete\n");
        return false;
    }
    for (i = 0; i != n->unk_04; i++) {
        if (!n->child[i] || !n->child[i]->val.isNumeric()) {
            printf("invalid arg\n");
            return false;
        }
    }
    switch (n->unk_04) {
    case 2: {
        mValue *a = &n->child[0]->val;
        mValue *b = &n->child[1]->val;
        switch (n->unk_02) {
        case 2:  return func_0804ca94(v, a, b);
        case 3:  return func_0804cd68(v, a, b);
        case 4:  return func_0804d040(v, a, b);
        case 5:  return func_0804d314(v, a, b);
        case 6:  return func_0804d5ec(v, a, b);
        case 8:  return func_0804d9c0(v, a, b);
        case 9:  return func_0804d7d4(v, a, b);
        case 10: return func_0804dfd4(v, a, b);
        case 11: return func_0804ddc0(v, a, b);
        case 12: return func_0804e050(v, a, b);
        case 13: return func_0804dbac(v, a, b);
        default:
            printf("unknown binary operation\n");
            return false;
        case 16: return func_0804e0cc(v, a, b);
        }
    }
    case 0: {
        if (!xpNodeDecode(n))
            return false;
        return v->isNumeric();
    }
    case 1: {
        mValue *a = &n->child[0]->val;
        switch (n->unk_02) {
        case 1:
        case 14: return v->set(a);
        case 7:  return v->bitNot(a);
        case 15: return v->negate(a);
        default:
            printf("bad unary operand\n");
            return false;
        case 17: return v->logicalNot(a);
        case 18: return v->abs(a);
        }
    }
    default:
        printf("unknownn n-ary operation\n");
        return false;
    }
}

bool func_0804bfcc(const char *a, const char *b);
char *dupString(const char *s);

xun *g_080bcda4;
int g_080bcda8;
int g_080b3140 = 0;

xpOp *func_0804f6c0(int type)
{
    for (int i = 0; i != 20; i++)
        if (opTable_080b30a0[i].type == type)
            return &opTable_080b30a0[i];
    return 0;
}

void func_0804f6ec()
{
    while (g_080bcda4) {
        xun *p = g_080bcda4;
        g_080bcda4 = p->next;
        delete (xunDel *)p;
    }
    g_080bcda4 = 0;
    g_080bcda8 = 0;
}

inline xun::xun(InitTag) { next = 0; name = 0; }

bool func_0804f75c(const char *s)
{
    if (!s) {
        func_0804ad38("passed a null string to add unknown\n");
        return false;
    }
    for (xun *p = g_080bcda4; p; p = p->next)
        if (func_0804bfcc(p->name, s))
            return true;
    xun *x = new xun(xun::INIT);
    if (!x) {
        func_0804ad38("failed to new xun\n");
        return false;
    }
    x->name = dupString(s);
    if (!x->name) {
        func_0804ad38("failed to dup string for xun\n");
        delete (xunDel *)x;
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

xun::xun()
{
    next = 0;
    name = 0;
}

xun::~xun()
{
    delete[] name;
    name = 0;
    next = 0;
}

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
