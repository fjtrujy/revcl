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

    infoNode *next;
};

class iHolder {
public:
    Instr *instruction;
    int operandIndex;
    int componentMask;

    inline iHolder();
    int matchesOperandLocation(ol_s *ol);
    void dumpOperandLocation();
    bool catInit(ol_s *ol);
    inline void dump();
};

class infoList {
public:
    infoList();
    virtual ~infoList();
    bool clear();
    void dumpEntries();
    bool addStall(ol_s *a, ol_s *b, int x, int val);

    infoNode *head;
    int count;
};

class stallNode : public infoNode {
public:
    enum InitTag { INIT };
    stallNode();
    inline stallNode(InitTag);
    virtual ~stallNode();
    virtual void dump();
    bool catStall(ol_s *a, ol_s *b, int x, int val);

    bool isLocal;
    iHolder setter;
    iHolder user;
    unsigned short stallByType[3];
};

class RegPoolEntry;

class ubsNode : public infoNode {
public:
    ubsNode();
    virtual ~ubsNode();
    virtual void dump();
    bool catInit(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits);

    RegPoolEntry *reg;
    unsigned char slotMask;
    instStream *entryStream;
    instStream *useStream;
};

#endif
