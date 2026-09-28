// Expression tree node (class name guessed), sizeof 0x1c, no vptr.
#ifndef XPNODE_H
#define XPNODE_H

#include "mValue.h"
#include "os.h"

class xpNode {
public:
    char unk_00;
    unsigned char unk_01;
    char unk_02;  // non-literal flag? (see 0x0804e80c)
    unsigned char unk_03;  // precedence
    unsigned char unk_04;
    unsigned char unk_05;
    xpNode *parent;
    xpNode *child[2];
    mValue val;

    xpNode();
    ~xpNode();
    bool func_0804e778();
    void func_0804e794();
    void reset();
    bool isFull();
    bool decode();
    const char *getLiteral();

    void clearValue() { val.clear(); }

    void init()
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
};

// Owner of an expression tree (class name guessed), sizeof 0x10, no vptr.
class xpTree {
public:
    xpNode *cur;
    xpNode *root;
    int unk_08;
    int unk_0c;

    xpTree();
    bool dispose(xpNode *n);
    bool reset();
    void calcDepths();
    inline void setDepth(xpNode *n, int depth);
    void func_0804ea8c(xpNode *n);
    void func_0804ead0();
    void dumpNode(xpNode *n);
    void dump();
    bool evalExp(const char *s);

    bool clear()
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
