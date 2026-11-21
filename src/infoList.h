// iHolder (instruction/operand/field holder) and infoList base class, CIE group 3.
#ifndef INFOLIST_H
#define INFOLIST_H

#include "Instr.h"
#include "os.h"
#include "ol_s.h"

class infoNode {
public:
    enum InitTag { INIT };
    infoNode();
    inline infoNode(InitTag);
    virtual ~infoNode() = 0;
    virtual void dump() = 0;

    infoNode *unk_04;
};

class iHolder {
public:
    Instr *unk_00;
    int unk_04;
    int unk_08;

    inline iHolder();
    int func_08052380(ol_s *ol);
    void func_080523b8();
    bool catInit(ol_s *ol);
    inline void dump();
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

class stallNode : public infoNode {
public:
    enum InitTag { INIT };
    stallNode();
    inline stallNode(InitTag);
    virtual ~stallNode();
    virtual void dump();
    bool catStall(ol_s *a, ol_s *b, int x, int val);

    bool unk_08;
    iHolder unk_0c;
    iHolder unk_18;
    unsigned short unk_24[3];
};

class RegPoolEntry;

class ubsNode : public infoNode {
public:
    ubsNode();
    virtual ~ubsNode();
    virtual void dump();
    bool catInit(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits);

    RegPoolEntry *unk_08;   // register
    unsigned char unk_0c;   // slot mask
    instStream *unk_10;     // entry point
    instStream *unk_14;     // use point
};

#endif
