// List of (src key, dst key) pairs, CIE group 5.
#ifndef TPAIRLIST_H
#define TPAIRLIST_H

#include "tPair.h"

class tPairList {
public:
    int count;
    tPair *head;

    tPairList()
    {
        count = 0;
        head = 0;
    }
    ~tPairList() { clear(); }
    bool clear();
    void dump();
    bool addPair(void *a, void *b, int src, int dst);
};

#endif
