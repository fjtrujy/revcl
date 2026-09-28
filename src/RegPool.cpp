#include "RegPool.h"

RegPool::RegPool()
{
    unk_28 = 0;
    unk_2c = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_18 = 0;
    unk_20 = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_10 = 0;
    unk_14 = 0;
    unk_0e = 0;
}

RegPool::~RegPool()
{
    RegPoolEntry* p = unk_1c;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    unk_1c = unk_20 = 0;
    unk_24 = 0;
    delete[] unk_04;
}
