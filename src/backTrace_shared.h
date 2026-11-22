#ifndef BACKTRACE_SHARED_H
#define BACKTRACE_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"

bool initializeStreamBacktrace(instStream *s, int n);
bool extendBackTrace(Program *p, instStream *n);
bool addNewActive(Program *p, instStream *n, instStream *from);
bool recalcMinTraceLengths(Program *p);
bool recalcMaxTraceLengths(Program *p);
int mergeBacktraceTraceStores(Program *p, unsigned *dst, unsigned *src);
int buildMinimumPathKey(Program *p, instStream *n);
int getMaxPathKey(Program *p, instStream *n);
bool isPathRaw(Program *p, int key);
bool buildCrossStreamThroughputPairs(Program *p, RegPoolEntry *registerEntry, int part, instStream *t, void *b);
bool addThroughputStalls(Program *p, instStream *t, RegPoolEntry *registerEntry, int part);
bool linkMatchList(Program *p, instStream *t, RegPoolEntry *registerEntry, int part, int ofs, bool ext);
bool processActiveList(Program *p);
bool initializeRegisterBacktraceLocation(instStream *n, RegPoolEntry *registerEntry, int part);
void dumpProgramStreamsAndRegisters(Program *p, int a);

#endif
