// Expression tree node (class name guessed), sizeof 0x1c, no vptr.
#ifndef XPNODE_H
#define XPNODE_H

#include <stdio.h>
#include "mValue.h"
#include "os.h"

class xpNode {
public:
    char unk_00;
    unsigned char depth;
    unsigned char operatorType;
    unsigned char precedence;
    unsigned char operandArity;
    unsigned char childCount;
    xpNode *parent;
    xpNode *child[2];
    mValue val;

    xpNode();
    ~xpNode();
    bool clearExpressionNodeValue();
    void dumpExpressionNodeValue();
    void reset();
    bool isFull();
    bool decode();
    const char *getLiteral();

    inline void clearValue();
    inline void init();
};

// Owner of an expression tree (class name guessed), sizeof 0x10, no vptr.
class xpTree {
public:
    xpNode *cur;
    xpNode *root;
    int unk_08;
    int unk_0c;

    xpTree();
    inline ~xpTree();
    bool dispose(xpNode *n);
    bool reset();
    void calcDepths();
    void setDepth(xpNode *n, int depth);
    void visitExpressionDescendants(xpNode *n);
    void resetExpressionDepths();
    void dumpNode(xpNode *n);
    void dump();
    bool evalExp(const char *s);
    bool evaluate(mValue *out, unsigned int type);
    bool evalUInt(unsigned int *out);
    bool evalFloat(float *out);
    bool evalInt(int *out);
    bool rec_Eval(xpNode *n);
    int countUnknownExpressionSymbols();
    void countUnknownSymbolsInNode(xpNode *n);

    inline bool clear();
};

// Expression operator table entry (name guessed).
struct xpOp {
    const char *name;
    unsigned char type;   // -> xpNode::unk_02
    unsigned char arity;  // -> xpNode::unk_04
    unsigned char prec;   // -> xpNode::unk_03
};

extern xpOp expressionOperatorTable[20];

const char *parseExpressionTokenIntoNode(const char *s, xpNode *node, bool binary);

#endif
