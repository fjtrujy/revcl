#ifndef WRANGLE_SHARED_H
#define WRANGLE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;
extern int g_080de758;
extern volatile char g_080de75c;
extern int errorOccurred;
extern bool quiet_080bbd98;

bool func_0807f560(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool func_0806558c(Instr *i, int *row);
bool func_0807f2c4(Block *blk, Instr *mv);
bool func_08080460(workTable *wt, Instr *in, int *row, int *pipe);
bool wrangleMode0Worker(workTable *wt, wrangleFunc f);
bool func_08081610(workTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(workTable *wt);
bool func_0808158c(workTable *wt, wrangleCand *c);
int checkInsert(workTable *wt, Instr *in, cls08058718 *pos, int *minPos);
bool buildCandidateList(workTable *wt);
bool func_080803dc(workTable *wt, wrangleCand *c);
bool DoInsert(workTable *wt, Instr *in, cls08058718 *pos);
bool UndoInsert(workTable *wt, Instr *in);
int drag(workTable *wt, Instr *in, cls08058718 *pos, int back, int depth);
int func_0807de00(workTable *wt, Instr *in, cls08058718 *pos);
int func_0807e1f8(workTable *wt, Instr *in, cls08058718 *pos);
int getMove(Block *blk, Instr *in, int now);
bool undoMovesTo(Block *blk, Instr *stop);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);

#endif
