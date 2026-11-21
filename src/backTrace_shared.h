#ifndef BACKTRACE_SHARED_H
#define BACKTRACE_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"

bool g_initBackTrace(instStream *s, int n);
bool extendBackTrace(Program *p, instStream *n);
bool addNewActive(Program *p, instStream *n, instStream *from);
bool recalcMinTraceLengths(Program *p);
bool recalcMaxTraceLengths(Program *p);
int mergeTstores(Program *p, unsigned *dst, unsigned *src);
int func_08061cb0(Program *p, instStream *n);
int getMaxPathKey(Program *p, instStream *n);
bool isPathRaw(Program *p, int key);
bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b);
bool addThruputStalls(Program *p, instStream *t, RegPoolEntry *reg, int part);
bool linkMatchList(Program *p, instStream *t, RegPoolEntry *reg, int part, int ofs, bool ext);
bool processActiveList(Program *p);
bool func_08061ab0(instStream *n, RegPoolEntry *reg, int part);
void func_0806ed78(Program *p, int a);

#endif
