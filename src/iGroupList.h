#ifndef IGROUPLIST_H
#define IGROUPLIST_H

struct iGroupNode {
    iGroupNode *next;
    void *itemPointer;
    int itemSize;

    enum InitTag { INIT };
    iGroupNode();
    inline iGroupNode(InitTag);
    ~iGroupNode();
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
