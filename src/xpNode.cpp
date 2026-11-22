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

xpOp expressionOperatorTable[20] = {
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
    depth = 0;
    operatorType = 0;
    unmatchedOpeningParentheses = 0;
    operandArity = 0;
    childCount = 0;
    precedence = 0;
    val.clear();
    parent = 0;
    child[0] = 0;
    child[1] = 0;
    val.clear();
}

inline bool xpTree::clear()
{
    if (!dispose(root)) {
        reportError("failed to dispose xn\n");
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

bool xpNode::clearExpressionNodeValue()
{
    clearValue();
    return true;
}

void xpNode::dumpExpressionNodeValue()
{
    val.dump();
}

void xpNode::reset()
{
    init();
}

static inline bool xpNodeIsFull(xpNode *n) { return n->childCount >= n->operandArity; }
static inline bool xpNodeDecode(xpNode *n)
{
    if (n->operatorType) {
        printf("can't decode a non-literal\n");
        return false;
    }
    if (!n->val.makeNumeric()) {
        reportError("couldn't convert to arithmetic\n");
        return false;
    }
    return true;
}

bool xpNode::isFull()
{
    return childCount >= operandArity;
}

bool xpNode::decode()
{
    if (operatorType) {
        printf("can't decode a non-literal\n");
        return false;
    }
    if (!val.makeNumeric()) {
        reportError("couldn't convert to arithmetic\n");
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
    for (int i = 0; i != n->childCount; i++)
        if (!dispose(n->child[i]))
            return false;
    delete (xpNodeDel *)n;
    return true;
}

bool xpTree::reset()
{
    if (!dispose(root)) {
        reportError("failed to dispose xn\n");
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
        n->depth = 0;
        for (int i = 0; i != n->childCount; i++)
            setDepth(n->child[i], 1);
    }
}

void xpTree::setDepth(xpNode *n, int depth)
{
    if (n) {
        n->depth = depth;
        const int &d = depth;
        int i = 0;
        if (n->childCount) {
            int childDepth = d + 1;
            do {
                setDepth(n->child[i], childDepth);
                i++;
            } while (i != n->childCount);
        }
    }
}

static inline void setDepthInl(xpTree *t, xpNode *n, int depth)
{
    if (n) {
        n->depth = depth;
        const int &d = depth;
        int i = 0;
        if (n->childCount) {
            int childDepth = d + 1;
            do {
                t->setDepth(n->child[i], childDepth);
                i++;
            } while (i != n->childCount);
        }
    }
}

void xpTree::visitExpressionDescendants(xpNode *n)
{
    if (n && n->operandArity)
        for (int i = 0; i != n->operandArity; i++)
            visitExpressionDescendants(n->child[i]);
}

void xpTree::resetExpressionDepths()
{
    unk_0c = 0;
    xpNode *n = root;
    if (n) {
        n->depth = 0;
        for (int i = 0; i != n->childCount; i++)
            setDepthInl(this, n->child[i], 1);
    }
    visitExpressionDescendants(root);
}

void xpTree::dumpNode(xpNode *n)
{
    if (!n)
        return;
    for (int i = 0; i != n->depth; i++)
        ;
    n->val.dump();
    for (int i = 0; i != n->operandArity; i++) {
        if (n->child[i])
            dumpNode(n->child[i]);
        else
            reportError("null arg\n");
    }
}

void xpTree::dump()
{
    unk_0c = 0;
    xpNode *n = root;
    if (n) {
        n->depth = 0;
        for (int i = 0; i != n->childCount; i++)
            setDepthInl(this, n->child[i], 1);
    }
    dumpNode(root);
}

const char *parseExpressionTokenIntoNode(const char *s, xpNode *node, bool binary)
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
            xpOp *e = &expressionOperatorTable[i];
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
        found = &expressionOperatorTable[0];
    } else {
        if (!found)
            return 0;
        s = found->name;
        for (const char *q = s; *q; q++, p++)
            len++;
    }
    if (!node->val.setString(s, len))
        return 0;
    node->operatorType = found->type;
    node->precedence = found->prec;
    node->operandArity = found->arity;
    return p;
}

static inline xpOp *findOp(unsigned char type)
{
    for (int i = 0; i != 20; i++)
        if (expressionOperatorTable[i].type == type)
            return &expressionOperatorTable[i];
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
    root->operandArity = op->arity;
    root->precedence = op->prec;
    root->operatorType = op->type;
    if (!root->val.setString("=")) {
        reportError("not set equ op\n");
        return false;
    }
    cur = root;
    p = s;
    while (*p) {
        if (*p == '(') {
            if (cur->unmatchedOpeningParentheses == -1) {
                reportError("too many open brackets!\n");
                goto fail;
            }
            cur->unmatchedOpeningParentheses++;
            p++;
        } else if (*p == ')') {
            if (cur->childCount < cur->operandArity) {
                reportError("unexpected close bracket\n");
                return false;
            }
            while (cur && cur->unmatchedOpeningParentheses == 0)
                cur = cur->parent;
            if (!cur) {
                reportError("unbalanced brackets\n");
                return false;
            }
            cur->unmatchedOpeningParentheses--;
            p++;
        } else {
            xpNode *n = new xpNode;
            if (!n) {
                reportError("failed to get new xn .. bah\n");
                goto fail;
            }
            p = parseExpressionTokenIntoNode(p, n, xpNodeIsFull(cur));
            if (!p) {
                delete (xpNodeDel *)n;
                goto fail;
            }
            if (cur->childCount >= cur->operandArity) {
                if (n->operandArity <= 1) {
                    reportError("evalExp: syntax error at : nArgs=%d\n", n->operandArity);
                    n->val.dump();
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                while (cur && n->precedence <= cur->precedence && cur->unmatchedOpeningParentheses == 0)
                    cur = cur->parent;
                if (!cur || cur->childCount == 0) {
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                xpNode *last = cur->child[cur->childCount - 1];
                if (last->parent != cur) {
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                n->parent = cur;
                n->child[0] = last;
                n->childCount = 1;
                last->parent = n;
                cur->child[cur->childCount - 1] = n;
                cur = n;
            } else {
                if (n->operandArity > 1) {
                    delete (xpNodeDel *)n;
                    goto fail;
                }
                cur->child[cur->childCount] = n;
                cur->childCount++;
                n->parent = cur;
                if (n->operandArity)
                    cur = n;
            }
        }
    }
    for (; cur; cur = cur->parent)
        if (cur->unmatchedOpeningParentheses != 0)
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
        reportError("asked to evaluate a tree as a literal!\n");
        reportError("code this later\n");
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
        reportError("rec_Eval::invalid xpValue\n");
        return false;
    }
    int i;
    for (i = 0; i != n->operandArity; i++)
        if (!rec_Eval(n->child[i]))
            return false;
    if (n->childCount != n->operandArity) {
        printf("tree is incomplete\n");
        return false;
    }
    for (i = 0; i != n->operandArity; i++) {
        if (!n->child[i] || !n->child[i]->val.isNumeric()) {
            printf("invalid arg\n");
            return false;
        }
    }
    switch (n->operandArity) {
    case 2: {
        mValue *a = &n->child[0]->val;
        mValue *b = &n->child[1]->val;
        switch (n->operatorType) {
        case 2:  return addValues(v, a, b);
        case 3:  return subtractValues(v, a, b);
        case 4:  return multiplyValues(v, a, b);
        case 5:  return divideValues(v, a, b);
        case 6:  return moduloValues(v, a, b);
        case 8:  return shiftValuesRight(v, a, b);
        case 9:  return shiftValuesLeft(v, a, b);
        case 10: return bitwiseAndValues(v, a, b);
        case 11: return logicalAndValues(v, a, b);
        case 12: return bitwiseOrValues(v, a, b);
        case 13: return logicalOrValues(v, a, b);
        default:
            printf("unknown binary operation\n");
            return false;
        case 16: return bitwiseXorValues(v, a, b);
        }
    }
    case 0: {
        if (!xpNodeDecode(n))
            return false;
        return v->isNumeric();
    }
    case 1: {
        mValue *a = &n->child[0]->val;
        switch (n->operatorType) {
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

bool asciiCaseEqual(const char *a, const char *b);
char *dupString(const char *s);

xun *unknownSymbolListHead;
int unknownSymbolCount;
int expressionUnknownSymbolCount = 0;

xpOp *findExpressionOperatorByType(int type)
{
    for (int i = 0; i != 20; i++)
        if (expressionOperatorTable[i].type == type)
            return &expressionOperatorTable[i];
    return 0;
}

void clearUnknownSymbols()
{
    while (unknownSymbolListHead) {
        xun *p = unknownSymbolListHead;
        unknownSymbolListHead = p->next;
        delete (xunDel *)p;
    }
    unknownSymbolListHead = 0;
    unknownSymbolCount = 0;
}

inline xun::xun(InitTag) { next = 0; name = 0; }

bool recordUnknownSymbol(const char *s)
{
    if (!s) {
        reportError("passed a null string to add unknown\n");
        return false;
    }
    for (xun *p = unknownSymbolListHead; p; p = p->next)
        if (asciiCaseEqual(p->name, s))
            return true;
    xun *x = new xun(xun::INIT);
    if (!x) {
        reportError("failed to new xun\n");
        return false;
    }
    x->name = dupString(s);
    if (!x->name) {
        reportError("failed to dup string for xun\n");
        delete (xunDel *)x;
        return false;
    }
    unknownSymbolCount++;
    x->next = unknownSymbolListHead;
    unknownSymbolListHead = x;
    return true;
}

void visitUnknownSymbols()
{
    for (xun *p = unknownSymbolListHead; p; p = p->next)
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

int xpTree::countUnknownExpressionSymbols()
{
    expressionUnknownSymbolCount = 0;
    xpNode *n = root;
    if (n)
        countUnknownSymbolsInNode(n);
    return expressionUnknownSymbolCount;
}

void xpTree::countUnknownSymbolsInNode(xpNode *n)
{
    if (expressionUnknownSymbolCount < 0)
        return;
    if (n->operandArity) {
        for (int i = 0; i != n->operandArity; i++) {
            if (n->child[i]) {
                countUnknownSymbolsInNode(n->child[i]);
                if (expressionUnknownSymbolCount < 0)
                    return;
            }
        }
    } else {
        if (n->val.makeNumeric())
            return;
        const char *s = n->val.getString();
        if (!s)
            s = "<no_literal>";
        if (s && !recordUnknownSymbol(s)) {
            reportError("failed to add an unknown\n");
            expressionUnknownSymbolCount = -1;
            return;
        }
        expressionUnknownSymbolCount++;
    }
}
