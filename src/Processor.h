// Processor description (CIE group 18).
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "mList.h"
#include "ubsList.h"

class stallList : public infoList {
};

class iInfo;
class RegPool;
class Instr;
class LocTag;
struct ol_s;

struct PoolDef {            // array ends at type == 0
    int type;               // 1 gen, 2 hw, 3 mem
    int unk_04;
    const char *name;
    int cls;                // register class, index into g_080de6e0
    int unk_10;
    const char *zeroReg;
    const char *extReg;
};

struct CpuDef {
    const char *name;
    int unk_04;             // instructions per raw line
    PoolDef *pools;         // strap table
    void *devtab;
    iInfo *iinfos;          // itable
    void *srcSpecs;
    void *dstSpecs;
};

extern int g_080b33a8;
extern iInfo *g_080de6c0;
extern RegPool *g_080de6e0[12];
extern int g_080de714[3];
extern void *g_080de720;    // device table
extern void *g_080de724;    // srcSpecs
extern void *g_080de728;    // dstSpecs

class Processor {
public:
    Processor();
    ~Processor();
    bool Init(CpuDef *cpu);
    bool func_0806bc84();
    void func_0806bcb8();
    int gatherInflight(ol_s **out);
    bool func_0806bd40();
    bool func_0806bd88();
    void func_0806bdd0();

    bool setUniqueName(const char *name);
    const char *getUniqueName();
    Instr *getNextInstruction(LocTag *t, int *pos);
    bool decodeLE(LocTag *t, int *pos, mList *labels, Instr **out, int *nOut);

    bool unk_00;           // new syntax
    mList unk_04;
    const char *unk_18;    // global name (dupString)
    const char *unk_1c;    // name, default "Nameless"
    mList unk_20;
    char *unk_34;          // unique name (dupString), default "FLOW"
    int unk_38;
    char unk_3c;
    char unk_3d;           // mode: 1 normal, 3 raw (VSM)
    char unk_3e;           // 3 reorder, 4 noreorder
    bool unk_3f;           // inside .rawloop
    short unk_40;          // rawloop count
    int unk_44;            // mpg target address, -1 = unset
    int unk_48;
    mList unk_4c;
    char unk_60;           // unique all
    char unk_61;           // saved unk_3e
    char unk_62;           // saved unk_3d
    bool unk_63;
    char unk_64;
    char unk_65;
    ubsList unk_68;        // used-before-set list; unk_08 = count
    stallList unk_74;      // stall list; unk_08 = count
};

#endif
