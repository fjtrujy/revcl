#ifndef STALLNODE_INLINE_H
#define STALLNODE_INLINE_H
#pragma interface

class stallNode : public infoNode {
public:
    stallNode() { for (int i = 0; i != 3; i++) unk_24[i] = 0; }
    virtual ~stallNode();
    virtual void dump();
    bool catStall(ol_s *a, ol_s *b, int x, int val);

    bool unk_08;
    iHolder unk_0c;
    iHolder unk_18;
    unsigned short unk_24[3];
};

#endif
