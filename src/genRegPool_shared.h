#ifndef GENREGPOOL_SHARED_H
#define GENREGPOOL_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *GetRP(unsigned t);
bool resetProgramAllocationsForFirstPass(Program *p);
bool resetProgramAllocationsForSecondPass(Program *p);
bool setRange(int start, int end, int lsz);
int countSetBits(unsigned int);
int setRangeFrom(workTable *t);
static inline int bitSet(int m, int i)
{
    return (m >> i) & 1;
}
bool initRegMask(RegPool *pool);
int firstEmptyMask(RegPool *pool);
bool regRefLiveRange(regUse *ru, int part, int *s, int *e, bool *always);
bool hack_slo(workTable *wt, RegPoolEntry *a, RegPoolEntry *b);
extern int maxParts_080bce20;
extern int nRegs_080bce24;
extern int tStart_080bce28;
extern int tEnd_080bce2c;
extern char aTab_080bce40[0x20000];
extern char tTab_080dce40[0x1000];
extern char regMask_080dde40[32];

#endif
