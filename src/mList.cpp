// Intrusive doubly linked list (CIE group 1, after os.cpp).

#include "mList.h"

mList::mList()
{
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
}

mList::~mList()
{
    func_0804b1b0();
}

bool mList::func_0804b1b0()
{
    mNode *node = unk_0c;
    while (node) {
        mNode *next = node->unk_04;
        if (!node->detach())
            return false;
        if (!node->func_0804b75c())
            return false;
        node = next;
    }
    return true;
}

mNode *mList::func_0804b20c(void *data)
{
    mNode *node = unk_0c;
    while (node && node->unk_10 != data)
        node = node->unk_04;
    return node;
}
