#ifndef GENREGPOOL_SHARED_H
#define GENREGPOOL_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *getRegisterPoolByIndex(unsigned t);
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
bool haveSimultaneousRegisterOutputs(workTable *wt, RegPoolEntry *a, RegPoolEntry *b);
extern int maximumAllocationParts;
extern int allocationRegisterCount;
extern int allocationTimelineStart;
extern int allocationTimelineEnd;
extern char allocationRegisterTimeline[0x20000];
extern char candidateRegisterTimeline[0x1000];
extern char allocationRegisterMask[32];

#endif
