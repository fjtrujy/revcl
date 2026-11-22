#ifndef IGROUPLIST_H
#define IGROUPLIST_H

struct InstructionGroupNode {
    InstructionGroupNode *next;
    void *itemPointer;
    int itemSize;

    enum InitTag { INIT };
    InstructionGroupNode();
    inline InstructionGroupNode(InitTag);
    ~InstructionGroupNode();
};

// "igrouplist::init - first set!"
class InstructionGroupList {
public:
    InstructionGroupNode *head;

    InstructionGroupList();
    ~InstructionGroupList();
    void init();
    void clear();
    bool add(void *p, int sz);
    void dump();
};

#endif
