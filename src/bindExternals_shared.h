#ifndef BINDEXTERNALS_SHARED_H
#define BINDEXTERNALS_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"

Instr *addXinFor(instStream *s, RegPoolEntry *r);
Instr *addXoutFor(instStream *s, RegPoolEntry *r);
bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag);
RegPoolEntry *getTagRegister(RegPool *pool, char *name);
int getPoolsByClassID(int cls, RegPool **tab, int max);
RegPool *GetRP(unsigned t);
const char *getUniqueName(Program *p);
int mstrlen(const char *s);
bool expandRawTables(Program *p);
bool resolveStreamBranches(Program *p);
bool recalcFlowUIDs(Program *p);
bool buildUta(Program *prog, instStream *s, flowBlock *blocks, int count);
bool buildSynth(Program *prog, instStream *s, flowBlock *blocks, int count);
bool bindDependencies(Program *p);

#endif
