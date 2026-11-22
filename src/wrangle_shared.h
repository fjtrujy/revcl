#ifndef WRANGLE_SHARED_H
#define WRANGLE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;
extern int mode0WatchdogSeconds;
extern volatile char mode0WatchdogExpired;
extern int errorOccurred;
extern bool quietDiagnostics;

bool wrangleMode1Worker(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool calculateEarliestDependencyRow(Instr *i, int *row);
bool queueMoveForResolution(Block *blk, Instr *mv);
bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe);
bool wrangleMode0Worker(workTable *wt, wrangleFunc f);
bool findInsertPositionMode0(workTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(workTable *wt);
bool refineMode0InsertionCandidate(workTable *wt, wrangleCand *c);
int checkInsert(workTable *wt, Instr *in, cls08058718 *pos, int *minPos);
bool buildCandidateList(workTable *wt);
bool refineMode1InsertionCandidate(workTable *wt, wrangleCand *c);
bool DoInsert(workTable *wt, Instr *in, cls08058718 *pos);
bool UndoInsert(workTable *wt, Instr *in);
int drag(workTable *wt, Instr *in, cls08058718 *pos, int back, int depth);
int pullSpecialRegisterWritersEarlier(workTable *wt, Instr *in, cls08058718 *pos);
int pullLatencyWritersEarlier(workTable *wt, Instr *in, cls08058718 *pos);
int getMove(Block *blk, Instr *in, int now);
bool undoMovesTo(Block *blk, Instr *stop);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);

#endif
