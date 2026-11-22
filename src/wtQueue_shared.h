#ifndef WTQUEUE_SHARED_H
#define WTQUEUE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int unk_10;
};

extern optimiser optTable[11];
extern const char *g_080de730;

instStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, instStream *s, bool b);
workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(workTable *wt);
bool func_0807f360(workTable *wt, wrangleFunc f);
bool func_0808061c(workTable *wt, wrangleFunc f);
bool reduceExtents(workTable *wt);
bool fiddleBDL(workTable *wt);
bool assignLocalRegisters(Program *p, workTable *wt);
bool makeLpWork(instStream *s, int maxSpread);
bool disposeLpWork(instStream *s);
bool keepLpWork(instStream *s);
bool optReset(Program *p);
workTable *makeForceTable(Program *p, instStream *s);
void dumpProgramStreamsAndRegisters(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(workTable *wt);
bool func_0807e528(Program *p, instStream *s, workTable *wt);
bool func_0807e740(Program *p, workTable *wt, unsigned char a, unsigned char b);
bool UnlinkFromDone(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);

#endif
