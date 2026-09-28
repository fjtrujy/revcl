// Expression tree node (class name guessed), sizeof 0x1c, no vptr.
#ifndef XPNODE_H
#define XPNODE_H

#include "mValue.h"

class xpNode {
public:
    char unk_00;
    char unk_01;
    char unk_02;  // non-literal flag? (see 0x0804e80c)
    char unk_03;
    unsigned char unk_04;
    unsigned char unk_05;
    int unk_08;
    int unk_0c;
    int unk_10;
    mValue val;

    xpNode();
    ~xpNode();
    bool func_0804e778();
    void func_0804e794();
    void reset();

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
        unk_08 = 0;
        unk_0c = 0;
        unk_10 = 0;
        val.clear();
    }
};

#endif
