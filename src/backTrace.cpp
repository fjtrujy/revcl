#pragma implementation "tStore.h"
#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"
#include "tStore.h"

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
void func_0806ed78(Program *p, int a);
