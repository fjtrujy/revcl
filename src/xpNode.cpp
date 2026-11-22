#include <stdio.h>
#include "xpNode.h"
#include "xun.h"
#include "os.h"

struct xpNodeDel {
    char pad[0x14];
    mValue val;
    ~xpNodeDel() { mValue *expressionValue = &val; expressionValue->clear(); }
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
    mValue *value = &val;
    value->clear();
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

static inline bool xpNodeIsFull(xpNode *node) { return node->childCount >= node->operandArity; }
static inline bool xpNodeDecode(xpNode *node)
{
    if (node->operatorType) {
        printf("can't decode a non-literal\n");
        return false;
    }
    if (!node->val.makeNumeric()) {
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
    char *literalText = val.getString();
    if (literalText)
        return literalText;
    return "<no_literal>";
}

xpTree::xpTree()
{
    root = 0;
    cur = 0;
    clear();
}

bool xpTree::dispose(xpNode *node)
{
    if (!node)
        return true;
    for (int i = 0; i != node->childCount; i++)
        if (!dispose(node->child[i]))
            return false;
    delete (xpNodeDel *)node;
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
    xpNode *node = root;
    if (node) {
        node->depth = 0;
        for (int i = 0; i != node->childCount; i++)
            setDepth(node->child[i], 1);
    }
}

void xpTree::setDepth(xpNode *node, int depth)
{
    if (node) {
        node->depth = depth;
        const int &d = depth;
        int i = 0;
        if (node->childCount) {
            int childDepth = d + 1;
            do {
                setDepth(node->child[i], childDepth);
                i++;
            } while (i != node->childCount);
        }
    }
}

static inline void setDepthInl(xpTree *expressionTree, xpNode *node, int depth)
{
    if (node) {
        node->depth = depth;
        const int &d = depth;
        int i = 0;
        if (node->childCount) {
            int childDepth = d + 1;
            do {
                expressionTree->setDepth(node->child[i], childDepth);
                i++;
            } while (i != node->childCount);
        }
    }
}

void xpTree::visitExpressionDescendants(xpNode *node)
{
    if (node && node->operandArity)
        for (int i = 0; i != node->operandArity; i++)
            visitExpressionDescendants(node->child[i]);
}

void xpTree::resetExpressionDepths()
{
    unk_0c = 0;
    xpNode *node = root;
    if (node) {
        node->depth = 0;
        for (int i = 0; i != node->childCount; i++)
            setDepthInl(this, node->child[i], 1);
    }
    visitExpressionDescendants(root);
}

void xpTree::dumpNode(xpNode *node)
{
    if (!node)
        return;
    for (int i = 0; i != node->depth; i++)
        ;
    node->val.dump();
    for (int i = 0; i != node->operandArity; i++) {
        if (node->child[i])
            dumpNode(node->child[i]);
        else
            reportError("null arg\n");
    }
}

void xpTree::dump()
{
    unk_0c = 0;
    xpNode *node = root;
    if (node) {
        node->depth = 0;
        for (int i = 0; i != node->childCount; i++)
            setDepthInl(this, node->child[i], 1);
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
    const char *tokenCursor = s;
    xpOp *found = 0;
    int state = 1;
    int dots = 0;
    do {
        char tokenCharacter = *tokenCursor;
        if (tokenCharacter == '(' || tokenCharacter == ')')
            break;
        switch (state) {
        case 1:
            if (tokenCharacter == '.') {
                if (dots)
                    state = 0;
                dots++;
            }
            else if (tokenCharacter == 'e')
                state = len ? 2 : 0;
            else if (!(tokenCharacter >= '0' && tokenCharacter <= '9') && tokenCharacter != '_')
                state = 0;
            break;
        case 2:
            if (tokenCharacter == '-' || tokenCharacter == '+')
                state = 3;
            else
                state = 0;
            break;
        case 3:
        default:
            state = 0;
        }
        for (int i = 2; i != 20; i++) {
            xpOp *operatorInfo = &expressionOperatorTable[i];
            if ((len || (binary ? operatorInfo->arity > 1 : operatorInfo->arity <= 1)) && state != 3) {
                const char *operatorCursor = operatorInfo->name, *inputCursor = tokenCursor;
                for (; *operatorCursor; operatorCursor++, inputCursor++)
                    if (*inputCursor != *operatorCursor)
                        break;
                if (!*operatorCursor) {
                    found = operatorInfo;
                    break;
                }
            }
        }
        if (found)
            break;
        len++;
        tokenCursor++;
    } while (*tokenCursor);
    if (len) {
        found = &expressionOperatorTable[0];
    } else {
        if (!found)
            return 0;
        s = found->name;
        for (const char *scanCursor = s; *scanCursor; scanCursor++, tokenCursor++)
            len++;
    }
    if (!node->val.setString(s, len))
        return 0;
    node->operatorType = found->type;
    node->precedence = found->prec;
    node->operandArity = found->arity;
    return tokenCursor;
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
    xpOp *operatorInfo = findOp(1);
    if (!operatorInfo) {
        puts("no xor for equality");
        goto fail;
    }
    root->operandArity = operatorInfo->arity;
    root->precedence = operatorInfo->prec;
    root->operatorType = operatorInfo->type;
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
            xpNode *node = new xpNode;
            if (!node) {
                reportError("failed to get new xn .. bah\n");
                goto fail;
            }
            p = parseExpressionTokenIntoNode(p, node, xpNodeIsFull(cur));
            if (!p) {
                delete (xpNodeDel *)node;
                goto fail;
            }
            if (cur->childCount >= cur->operandArity) {
                if (node->operandArity <= 1) {
                    reportError("evalExp: syntax error at : nArgs=%d\n", node->operandArity);
                    node->val.dump();
                    delete (xpNodeDel *)node;
                    goto fail;
                }
                while (cur && node->precedence <= cur->precedence && cur->unmatchedOpeningParentheses == 0)
                    cur = cur->parent;
                if (!cur || cur->childCount == 0) {
                    delete (xpNodeDel *)node;
                    goto fail;
                }
                xpNode *last = cur->child[cur->childCount - 1];
                if (last->parent != cur) {
                    delete (xpNodeDel *)node;
                    goto fail;
                }
                node->parent = cur;
                node->child[0] = last;
                node->childCount = 1;
                last->parent = node;
                cur->child[cur->childCount - 1] = node;
                cur = node;
            } else {
                if (node->operandArity > 1) {
                    delete (xpNodeDel *)node;
                    goto fail;
                }
                cur->child[cur->childCount] = node;
                cur->childCount++;
                node->parent = cur;
                if (node->operandArity)
                    cur = node;
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

bool xpTree::rec_Eval(xpNode *node)
{
    if (!node)
        return false;
    mValue *value = &node->val;
    if (!value->isValid()) {
        reportError("rec_Eval::invalid xpValue\n");
        return false;
    }
    int i;
    for (i = 0; i != node->operandArity; i++)
        if (!rec_Eval(node->child[i]))
            return false;
    if (node->childCount != node->operandArity) {
        printf("tree is incomplete\n");
        return false;
    }
    for (i = 0; i != node->operandArity; i++) {
        if (!node->child[i] || !node->child[i]->val.isNumeric()) {
            printf("invalid arg\n");
            return false;
        }
    }
    switch (node->operandArity) {
    case 2: {
        mValue *leftValue = &node->child[0]->val;
        mValue *rightValue = &node->child[1]->val;
        switch (node->operatorType) {
        case 2:  return addValues(value, leftValue, rightValue);
        case 3:  return subtractValues(value, leftValue, rightValue);
        case 4:  return multiplyValues(value, leftValue, rightValue);
        case 5:  return divideValues(value, leftValue, rightValue);
        case 6:  return moduloValues(value, leftValue, rightValue);
        case 8:  return shiftValuesRight(value, leftValue, rightValue);
        case 9:  return shiftValuesLeft(value, leftValue, rightValue);
        case 10: return bitwiseAndValues(value, leftValue, rightValue);
        case 11: return logicalAndValues(value, leftValue, rightValue);
        case 12: return bitwiseOrValues(value, leftValue, rightValue);
        case 13: return logicalOrValues(value, leftValue, rightValue);
        default:
            printf("unknown binary operation\n");
            return false;
        case 16: return bitwiseXorValues(value, leftValue, rightValue);
        }
    }
    case 0: {
        if (!xpNodeDecode(node))
            return false;
        return value->isNumeric();
    }
    case 1: {
        mValue *leftValue = &node->child[0]->val;
        switch (node->operatorType) {
        case 1:
        case 14: return value->set(leftValue);
        case 7:  return value->bitNot(leftValue);
        case 15: return value->negate(leftValue);
        default:
            printf("bad unary operand\n");
            return false;
        case 17: return value->logicalNot(leftValue);
        case 18: return value->abs(leftValue);
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
        xun *unknownSymbol = unknownSymbolListHead;
        unknownSymbolListHead = unknownSymbol->next;
        delete (xunDel *)unknownSymbol;
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
    for (xun *unknownSymbol = unknownSymbolListHead; unknownSymbol; unknownSymbol = unknownSymbol->next)
        if (asciiCaseEqual(unknownSymbol->name, s))
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
    for (xun *unknownSymbol = unknownSymbolListHead; unknownSymbol; unknownSymbol = unknownSymbol->next)
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
    xpNode *node = root;
    if (node)
        countUnknownSymbolsInNode(node);
    return expressionUnknownSymbolCount;
}

void xpTree::countUnknownSymbolsInNode(xpNode *node)
{
    if (expressionUnknownSymbolCount < 0)
        return;
    if (node->operandArity) {
        for (int i = 0; i != node->operandArity; i++) {
            if (node->child[i]) {
                countUnknownSymbolsInNode(node->child[i]);
                if (expressionUnknownSymbolCount < 0)
                    return;
            }
        }
    } else {
        if (node->val.makeNumeric())
            return;
        const char *literalText = node->val.getString();
        if (!literalText)
            literalText = "<no_literal>";
        if (literalText && !recordUnknownSymbol(literalText)) {
            reportError("failed to add an unknown\n");
            expressionUnknownSymbolCount = -1;
            return;
        }
        expressionUnknownSymbolCount++;
    }
}
