// iHolder (instruction/operand/field holder) and infoList base class, CIE group 3.
#ifndef INFOLIST_H
#define INFOLIST_H
#pragma interface

#include "Instr.h"
#include "os.h"

class infoNode;

struct ol_s {
    char unk_00[4];
    Instr *unk_04;  // instruction
    char unk_08[8];
    char unk_10;    // operand index
    char unk_11;    // field bit number
};

class iHolder {
public:
    Instr *unk_00;  // instruction
    int unk_04;     // operand index
    int unk_08;     // field bit mask

    inline iHolder();
    int func_08052380(ol_s *ol)
    {
        if (!ol || !unk_00)
            return 0;
        int r = 0;
        if (unk_00 == ol->unk_04 && unk_04 == ol->unk_10)
            r = 1;
        return r;
    }
    void func_080523b8();
    bool catInit(ol_s *ol);
    inline void dump();
};

class infoNode {
public:
    infoNode() { unk_04 = 0; }
    virtual ~infoNode() {}
    virtual void dump() = 0;

    infoNode *unk_04;  // next
};

class stallNode : public infoNode {
public:
    stallNode() { for (int i = 0; i != 3; i++) unk_24[i] = 0; }
    virtual ~stallNode() {}
    virtual void dump();
    bool catStall(ol_s *a, ol_s *b, int x, int val);

    bool unk_08;  // setter and user in same stream ("local")
    iHolder unk_0c;
    iHolder unk_18;
    unsigned short unk_24[3];
};

class RegPoolEntry;

class ubsNode : public infoNode {
public:
    ubsNode() { unk_08 = 0; unk_0c = 0; unk_10 = 0; unk_14 = 0; }
    virtual ~ubsNode() {}
    virtual void dump();

    RegPoolEntry *unk_08;
    unsigned char unk_0c;  // mask
    void *unk_10;
    void *unk_14;
};

class infoList {
public:
    infoList();
    virtual ~infoList();
    bool clear();
    void func_080525c8();
    bool addStall(ol_s *a, ol_s *b, int x, int val);

    infoNode *unk_04;  // head
    int unk_08;        // count
};

#endif
