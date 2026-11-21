#pragma implementation "RegWeb_inline.h"
#include "RegWeb_inline.h"

RegWeb regWebs[0x4000];
int regWebUsed;
int regWebCount;

#include "flowBlock.h"
#include "fileEmit.h"
#include "Processor.h"
char *emitReversibleBranch(char *buf, Instr *in, bool upper, char *mods, bool b, int);
bool extHook(void *, Instr *in);
bool func_08087860(void *, Instr *in);
bool efu_Hook(void *p, Instr *in);
bool wr2w_Hook(void *p, Instr *in);
char *emitClip(char *buf, Instr *in, bool upper, char *mods, bool b, int);
char *vmove_Emit(char *buf, Instr *in, bool upper, char *mods, bool b, int);
bool semSynth(void *p, Instr *in);

const char *pipeNames[16] = {
    "VPU_FMAC", "VPU_IALU", "VPU_MOVE", "VPU_LS", "VPU_EFU",
    "VPU_DIV", "VPU_RAND", "VPU_EXT", "VPU_BRANCH", "0",
};

RegDesc regDescTable[] = {
#include "regDescTable.h"
};

iInfo vuITable_080b3b20[] = {
#include "vuITable.h"
};

PoolDef vuPools_080bbae0[] = {
    {1, "vf", "VU User floating point", 9, 4, "vf00", 0},
    {1, "vi", "VU User integer", 8, 1, "vi00", 0},
    {2, "hw", "VU hardware register", 10, 0, 0, "EXT"},
    {0, 0, 0, 0, 0, 0, 0},
};

CpuDef cpu_080bbb50 = {
    "Marigold",
    2,
    vuPools_080bbae0,
    pipeNames,
    vuITable_080b3b20,
    opPairTable_080b32e0,
    opCountTable_080b3340,
};

