#ifndef IGROUPLIST_H
#define IGROUPLIST_H

#pragma interface

struct iGroupNode {
    iGroupNode *next;
    void *unk_04;
    int unk_08;     // size

    enum InitTag { INIT };
    iGroupNode();
    inline iGroupNode(InitTag);
    ~iGroupNode()
    {
    }
};

// "igrouplist::init - first set!"
class iGroupList {
public:
    iGroupNode *head;

    iGroupList();
    ~iGroupList();
    void init();
    void clear();
    bool add(void *p, int sz);
    void dump();
};

#endif
