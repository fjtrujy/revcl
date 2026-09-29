#pragma implementation "iGroupList.h"
#include "Instr.h"
#include "os.h"

iGroupList::iGroupList()
{
    head = 0;
}

iGroupList::~iGroupList()
{
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        delete n;
        n = next;
    }
    head = 0;
}

void iGroupList::init()
{
    if (head)
        func_0804ac7c("igrouplist::init - first set!\n");
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        delete n;
        n = next;
    }
    head = 0;
}

void iGroupList::clear()
{
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        delete n;
        n = next;
    }
    head = 0;
}

bool iGroupList::add(void *p, int sz)
{
    if (sz <= 0)
        return 1;
    iGroupNode *n = new iGroupNode;
    if (!n) {
        func_0804ad38("failed to get a new iGroupNode\n");
        return 0;
    }
    n->unk_04 = p;
    n->unk_08 = sz;
    n->next = head;
    head = n;
    return 1;
}

void iGroupList::dump()
{
    iGroupNode *n;
    int i = 0;
    for (n = head; n; n = n->next, i++)
        func_0804acb0(" [%4d ] [%d sz=%d]\n", i, n->unk_04, n->unk_08);
    if (i)
        func_0804acb0(" .. %d nodes\n", i);
}
