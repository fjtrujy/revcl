#ifndef TPAIR_H
#define TPAIR_H
#pragma interface

class tPair {
public:
    int src;
    int dst;
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
    void reset();
    void func_08058ab0();
};

#endif
