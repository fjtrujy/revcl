// List of (src key, dst key) pairs, CIE group 5.
#ifndef TPAIRLIST_H
#define TPAIRLIST_H
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
};

class tPairList {
public:
    int count;
    tPair *head;

    bool clear();
    void dump();
    bool addPair(void *a, void *b, int src, int dst);
};

#endif
