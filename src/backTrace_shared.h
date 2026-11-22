#ifndef BACKTRACE_SHARED_H
#define BACKTRACE_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"

bool initializeStreamBacktrace(InstructionStream *s, int n);
bool extendBackTrace(Program *p, InstructionStream *n);
bool addNewActive(Program *p, InstructionStream *n, InstructionStream *from);
bool recalcMinTraceLengths(Program *p);
bool recalcMaxTraceLengths(Program *p);
int mergeBacktraceTraceStores(Program *p, unsigned *dst, unsigned *src);
int buildMinimumPathKey(Program *p, InstructionStream *n);
int getMaxPathKey(Program *p, InstructionStream *n);
bool isPathRaw(Program *p, int key);
bool buildCrossStreamThroughputPairs(Program *p, RegisterEntry *registerEntry, int part, InstructionStream *t, void *b);
bool addThroughputStalls(Program *p, InstructionStream *t, RegisterEntry *registerEntry, int part);
bool linkMatchList(Program *p, InstructionStream *t, RegisterEntry *registerEntry, int part, int ofs, bool ext);
bool processActiveList(Program *p);
bool initializeRegisterBacktraceLocation(InstructionStream *n, RegisterEntry *registerEntry, int part);
void dumpProgramStreamsAndRegisters(Program *p, int a);

#endif
