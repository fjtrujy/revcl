// Expression operator/operand node (0x1c bytes on i386, no vptr).
#ifndef XPNODE_H
#define XPNODE_H

#include <stdio.h>
#include "mValue.h"
#include "os.h"

class ExpressionNode {
public:
    char unmatchedOpeningParentheses;
    unsigned char depth;
    unsigned char operatorType;
    unsigned char precedence;
    unsigned char operandArity;
    unsigned char childCount;
    ExpressionNode *parent;
    ExpressionNode *child[2];
    TypedValue val;

    ExpressionNode();
    ~ExpressionNode();
    bool clearExpressionNodeValue();
    void dumpExpressionNodeValue();
    void reset();
    bool isFull();
    bool decode();
    const char *getLiteral();

    inline void clearValue();
    inline void init();
};

// Expression tree owner and evaluator (0x10 bytes on i386, no vptr).
class ExpressionTree {
public:
    ExpressionNode *cur;
    ExpressionNode *root;
    int resetOnlyTreeStateWord0;
    int resetOnlyTreeStateWord1;

    ExpressionTree();
    inline ~ExpressionTree();
    bool dispose(ExpressionNode *n);
    bool reset();
    void calcDepths();
    void setDepth(ExpressionNode *n, int depth);
    void visitExpressionDescendants(ExpressionNode *n);
    void resetExpressionDepths();
    void dumpNode(ExpressionNode *n);
    void dump();
    bool evalExp(const char *s);
    bool evaluate(TypedValue *out, unsigned int type);
    bool evalUInt(unsigned int *out);
    bool evalFloat(float *out);
    bool evalInt(int *out);
    bool evaluateExpressionNode(ExpressionNode *n);
    int countUnknownExpressionSymbols();
    void countUnknownSymbolsInNode(ExpressionNode *n);

    inline bool clear();
};

// Expression operator descriptor (type, precedence and operand arity).
struct xpOp {
    const char *name;
    unsigned char type;   // matches ExpressionNode::operatorType
    unsigned char arity;  // matches ExpressionNode::operandArity
    unsigned char prec;   // matches ExpressionNode::precedence
};

extern xpOp expressionOperatorTable[20];

const char *parseExpressionTokenIntoNode(const char *s, ExpressionNode *node, bool binary);

#endif
