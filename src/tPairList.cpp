#pragma implementation "tPair.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"

bool tPairList::clear()
{
    while (head) {
        tPair *p = head;
        head = p->next;
        if (p)
            delete p;
    }
    count = 0;
    return true;
}

void tPairList::dump()
{
    int n = 0;
    for (tPair *p = head; p; p = p->next)
        n++;
    if (n != count) {
        func_0804ad38("     tpairdump : entry count mismatch\n");
        func_0804adf0(-1);
    }
}

bool tPairList::addPair(void *a, void *b, int src, int dst)
{
    if (!a || !b) {
        func_0804ad38("tPairList::addPair - null arg\n");
        return false;
    }
    if (src <= 0) {
        func_0804ad38("tPairList::addKey() src key is bad\n");
        return false;
    }
    if (dst < 0) {
        func_0804ad38("tPairList::addKey() dst_key is bad\n");
        return false;
    }
    tPair *p = new tPair;
    if (!p) {
        func_0804ad38("failed to get a new pair\n");
        return false;
    }
    p->a = a;
    p->b = b;
    p->src = src;
    p->dst = dst;
    p->next = head;
    count++;
    head = p;
    return true;
}
