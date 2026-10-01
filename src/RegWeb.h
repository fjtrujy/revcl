// Register webs. The class is not under #pragma interface (its inline ctor/dtor are
// not emitted out of line); deferred inlines live in RegWeb_inline.h.
#ifndef REGWEB_H
#define REGWEB_H

#include "os.h"
#include "Instr.h"
#include "workTable.h"

class RegWeb {
public:
    void *unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
    int unk_10;
    int unk_14;

    RegWeb() {}
    ~RegWeb() {}

    static bool AddiInstr(Instr *in);
    static bool AddiTable(workTable *t);
};

extern RegWeb regWebs[0x4000];
extern int regWebUsed;
extern int regWebCount;

bool resetRegWebs();

#endif
