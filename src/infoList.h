// iHolder (instruction/operand/field holder) and infoList base class, CIE group 3.
#ifndef INFOLIST_H
#define INFOLIST_H
#pragma interface

#include "Instr.h"
#include "os.h"
#include "ol_s.h"

class infoNode;

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
        Instr *i = ol->unk_04;
        if (unk_00 == i) {
            int idx = ol->unk_10;
            if (unk_04 == idx)
                r = 1;
        }
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
    ubsNode();
    virtual ~ubsNode() {}
    virtual void dump();
    bool catInit(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits);

    RegPoolEntry *unk_08;   // register
    unsigned char unk_0c;   // slot mask
    instStream *unk_10;     // entry point
    instStream *unk_14;     // use point
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
