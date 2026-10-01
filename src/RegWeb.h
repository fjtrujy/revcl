// Register webs. The class is not under #pragma interface (its inline ctor/dtor are
// not emitted out of line); deferred inlines live in RegWeb_inline.h.
#ifndef REGWEB_H
#define REGWEB_H

#include "os.h"
#include "Instr.h"
#include "workTable.h"

class RegWeb {
public:
    RegPoolEntry *unk_00;   // assigned register
    RegPoolEntry *unk_04;   // virtual register of the web
    RegPoolEntry *unk_08;   // forced (mapped) register
    RegWeb *unk_0c;         // parent link (merged into)
    unsigned short unk_10;  // count
    short unk_12;           // index in regWebs
    unsigned char unk_14;   // slot mask
    char unk_15[3];

    RegWeb() {}
    ~RegWeb() {}

    static bool AddiInstr(Instr *in);
    static RegWeb *GetNewWeb(RegPoolEntry *old_vrg);
    static bool merge(RegWeb *w_a, RegWeb *w_b);
    static bool setfMap(RegWeb *w, RegPoolEntry *r);
    static bool AddiTable(workTable *t);
    static bool PatchOperand(Operand *op);
    static bool PatchInstr(Instr *in);
    static bool PatchITable(workTable *t);
    static void Dump();
    static bool ClearWebs(workTable *t);
};

extern RegWeb regWebs[0x4000];
extern int regWebUsed;
extern int regWebCount;

bool resetRegWebs();
bool GatherWeb();

#endif
