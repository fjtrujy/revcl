// Intrusive doubly linked list (CIE group 1, after os.cpp).

#include "mList.h"
#include "os.h"

bool func_0804bfcc(const char *a, const char *b);
bool func_0804c03c(const char *a, const char *b);
char *dupString(const char *s);

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

mNode *mList::func_0804b234(const char *name)
{
    mNode *node = unk_0c;
    while (node) {
        if (name) {
            if (node->unk_14 && func_0804c03c(name, node->unk_14))
                break;
        } else if (!node->unk_14)
            break;
        node = node->unk_04;
    }
    return node;
}

mNode *mList::func_0804b284(const char *name)
{
    mNode *node = unk_0c;
    while (node) {
        if (name) {
            if (node->unk_14 && func_0804bfcc(name, node->unk_14))
                break;
        } else if (!node->unk_14)
            break;
        node = node->unk_04;
    }
    return node;
}

mNode *mList::func_0804b2d4(const char *name)
{
    mNode *node = func_0804b234(name);
    if (!node) {
        node = func_0804b444(name);
        if (!node) {
            func_0804ad38("create add tail failed.\n");
            func_0804adf0(-1);
        }
    }
    return node;
}

bool mList::func_0804b330(mNode *node)
{
    if (!node)
        return false;
    if (node->unk_0c)
        return false;
    node->unk_08 = 0;
    node->unk_04 = 0;
    node->unk_10 = (void *)++unk_04;
    unk_08++;
    node->unk_0c = this;
    if (unk_0c) {
        node->unk_04 = unk_0c;
        unk_0c->unk_08 = node;
    } else
        unk_10 = node;
    unk_0c = node;
    return true;
}

bool mList::func_0804b38c(mNode *node)
{
    if (!node)
        return false;
    if (node->unk_0c)
        return false;
    node->unk_08 = 0;
    node->unk_04 = 0;
    node->unk_10 = (void *)++unk_04;
    node->unk_0c = this;
    unk_08++;
    if (unk_10) {
        unk_10->unk_04 = node;
        node->unk_08 = unk_10;
        unk_10 = node;
    } else {
        unk_10 = node;
        unk_0c = node;
    }
    return true;
}

bool mList::func_0804b3e8(mNode *node)
{
    if (!node)
        return false;
    if (node->unk_0c != this)
        return false;
    node->unk_0c = 0;
    unk_08--;
    node->unk_10 = 0;
    if (node->unk_04)
        node->unk_04->unk_08 = node->unk_08;
    else
        unk_10 = node->unk_08;
    if (node->unk_08)
        node->unk_08->unk_04 = node->unk_04;
    else
        unk_0c = node->unk_04;
    return true;
}

mNode *mList::func_0804b444(const char *name)
{
    mNode *node = new mNode;
    if (!node)
        return 0;
    if (name)
        node->unk_14 = dupString(name);
    if (!func_0804b38c(node)) {
        delete node;
        return 0;
    }
    return node;
}

bool mList::func_0804b4d0(mList *other)
{
    mNode *node = other->unk_0c;
    while (node) {
        mNode *next = node->unk_04;
        if (!node->detach())
            return false;
        if (!func_0804b38c(node))
            return false;
        node = next;
    }
    return true;
}

void mList::func_0804b534()
{
    for (mNode *node = unk_0c; node; node = node->unk_04)
        ;
}

char *mList::func_0804b550()
{
    for (mNode *node = unk_0c; node; node = node->unk_04)
        if (node->unk_14)
            return node->unk_14;
    return 0;
}

char *mList::func_0804b574(char *buf)
{
    for (mNode *node = unk_0c; node; node = node->unk_04)
        if (node->unk_14)
            buf += func_0804adbc(buf, "%s:\n", node->unk_14);
    return buf;
}

mNode::~mNode()
{
    if (unk_14)
        delete[] unk_14;
    unk_14 = 0;
}
