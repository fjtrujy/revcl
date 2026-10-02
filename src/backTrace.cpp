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

static inline bool testPathBit(VisitCache *vc, int rec, int bit)
{
    unsigned *row = vc->unk_00 + rec * vc->unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void dumpPathKey(Program *p, int key)
{
    func_0804acb0("dumping path key '%d'\n", key);
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (testPathBit(&p->unk_bc, key, s->unk_2c0))
            func_0804acb0("    %d: %s\n", s->func_0806d674());
    }
    func_0804acb0(" done dumping path key '%d'\n", key);
}
