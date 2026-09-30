// Processor description (CIE group 18).
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "mList.h"
#include "ubsList.h"

class stallList : public infoList {
};

class iInfo;
class RegPool;
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

    char unk_00;
    mList unk_04;
    const char *unk_18;
    const char *unk_1c;    // name, default "Nameless"
    mList unk_20;
    int unk_34;
    int unk_38;
    char unk_3c;
    char unk_3d;
    char unk_3e;
    int unk_40;
    int unk_44;
    int unk_48;
    mList unk_4c;
    char unk_60;
    char unk_61[3];
    char unk_64;
    char unk_65;
    ubsList unk_68;
    stallList unk_74;
};

#endif
