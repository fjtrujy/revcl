#ifndef INFONODE_INLINE_H
#define INFONODE_INLINE_H
#pragma interface

class infoNode {
public:
    enum InitTag { INIT };
    infoNode();
    inline infoNode(InitTag);
    virtual ~infoNode() = 0;
    virtual void dump() = 0;

    infoNode *unk_04;
};

inline infoNode::~infoNode() {}

#endif
