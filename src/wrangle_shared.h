#ifndef WRANGLE_SHARED_H
#define WRANGLE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(ScheduleTable *, int *);

void adjustMoveCount(Block *block, Instr *mv, int n);
extern int optimizationTimeoutSeconds;
extern int mode0WatchdogSeconds;
extern volatile char mode0WatchdogExpired;
extern int errorOccurred;
extern bool quietDiagnostics;

bool wrangleMode1Worker(ScheduleTable *wt, wrangleFunc f);
bool dynCalcStats(ScheduleTable *wt);
bool calculateEarliestDependencyRow(Instr *i, int *row);
bool queueMoveForResolution(Block *block, Instr *mv);
bool findInsertPosition(ScheduleTable *wt, Instr *in, int *row, int *pipe);
bool wrangleMode0Worker(ScheduleTable *wt, wrangleFunc f);
bool findInsertPositionMode0(ScheduleTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(ScheduleTable *wt);
bool refineMode0InsertionCandidate(ScheduleTable *wt, wrangleCand *c);
int checkInsert(ScheduleTable *wt, Instr *in, InstructionPosition *position, int *minPos);
bool buildCandidateList(ScheduleTable *wt);
bool refineMode1InsertionCandidate(ScheduleTable *wt, wrangleCand *c);
bool insertInstructionWithLocks(ScheduleTable *wt, Instr *in, InstructionPosition *position);
bool undoInstructionWithLocks(ScheduleTable *wt, Instr *in);
int drag(ScheduleTable *wt, Instr *in, InstructionPosition *position, int back, int depth);
int pullSpecialRegisterWritersEarlier(ScheduleTable *wt, Instr *in, InstructionPosition *position);
int pullLatencyWritersEarlier(ScheduleTable *wt, Instr *in, InstructionPosition *position);
int getMove(Block *block, Instr *in, int now);
bool undoMovesTo(Block *block, Instr *stop);
bool resolveMovePipe(ScheduleTable *wt, Instr *mv, int pipe);

#endif
