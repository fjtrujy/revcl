// (src key, dst key) pair node, CIE group 5.
#ifndef TPAIR_H
#define TPAIR_H
#pragma interface

class tPair {
public:
    int src;        // src key
    int dst;        // dst key
    void *a;
    void *b;
    tPair *next;

    tPair()
    {
        next = 0;
        src = -1;
        dst = -1;
        a = 0;
        b = 0;
    }
    ~tPair() {}

    void reset()
    {
        next = 0;
        src = -1;
        dst = -1;
        a = 0;
        b = 0;
    }
    void func_08058ab0() {}
};

#endif
