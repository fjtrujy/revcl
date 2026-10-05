// Expression tree node (class name guessed), sizeof 0x1c, no vptr.
#ifndef XPNODE_H
#define XPNODE_H

#pragma interface

#include "mValue.h"
#include "os.h"

class xpNode {
public:
    char unk_00;
    unsigned char unk_01;
    unsigned char unk_02;  // operator type (xpOp::type), 0 = literal
    unsigned char unk_03;  // precedence
    unsigned char unk_04;
    unsigned char unk_05;
    xpNode *parent;
    xpNode *child[2];
    mValue val;

    xpNode();
    ~xpNode()
    {
        mValue *v = &val;
        v->clear();
    }
    bool func_0804e778();
    void func_0804e794();
    void reset();
    bool isFull() { return unk_05 >= unk_04; }
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
    inline void setDepth(xpNode *n, int depth);
    void func_0804ea8c(xpNode *n);
    void func_0804ead0();
    void dumpNode(xpNode *n);
    void dump();
    bool evalExp(const char *s);
    bool evaluate(mValue *out, unsigned int type);
    bool evalUInt(unsigned int *out);
    bool evalFloat(float *out);
    bool evalInt(int *out);
    bool rec_Eval(xpNode *n);
    int func_0804f8f0();
    void func_0804f920(xpNode *n);

    inline bool clear();
};

// Expression operator table entry (name guessed).
struct xpOp {
    const char *name;
    unsigned char type;   // -> xpNode::unk_02
    unsigned char arity;  // -> xpNode::unk_04
    unsigned char prec;   // -> xpNode::unk_03
};

extern xpOp opTable_080b30a0[20];

const char *func_0804ec74(const char *s, xpNode *node, bool binary);

#endif
