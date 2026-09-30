#include "Processor.h"
#include "Instr.h"
#include "genRegPool.h"
#include "hwRegPool.h"
#include "memRegPool.h"
#include "os.h"

extern int errorOccurred;
extern bool quiet_080bbd98;

bool func_08070180(iInfo *tab, int count);

Processor::Processor()
{
    unk_34 = 0;
    g_080b33a8 = 0;
    unk_00 = 0;
    g_080de6c0 = 0;
    unk_1c = "Nameless";
    unk_18 = "";
    unk_64 = 0;
    unk_65 = 0;
    unk_44 = -1;
    unk_38 = 0;
    unk_60 = 0;
    unk_3d = 0;
    unk_3e = 0;
    for (int i = 0; i != 12; i++)
        g_080de6e0[i] = 0;
    for (int i = 0; i != 3; i++)
        g_080de714[i] = 0;
}

Processor::~Processor()
{
}

bool Processor::Init(CpuDef *cpu)
{
    if (!cpu) {
        func_0804ad38("null pointer got into init for a cpu\n");
        return false;
    }
    if (!cpu->name) {
        func_0804ad38("must name a cpu\n");
        return false;
    }
    PoolDef *pools = cpu->pools;
    iInfo *iinfos = cpu->iinfos;
    if (!func_08070180(iinfos, 0xd6)) {
        func_0804ad38("couldnt init cpu '%s' - the idefs were bad\n", cpu->name);
        return false;
    }
    g_080de720 = cpu->devtab;
    unk_38 = cpu->unk_04;
    if (unk_38 < 1 || unk_38 > 2) {
        func_0804ad38("processor '%s' has too many instructions per raw line\n", cpu->name);
        return false;
    }
    unk_1c = cpu->name;
    if (!pools) {
        func_0804ad38("processor: init - passed a null strap table\n");
        return false;
    }
    if (!iinfos) {
        func_0804ad38("passed a null itable\n");
        return false;
    }
    g_080de6c0 = iinfos;
    for (int i = 0; pools[i].type; i++) {
        PoolDef *d = &pools[i];
        RegPool *pool;
        switch (d->type) {
        case 1:
            pool = new genRegPool;
            break;
        case 2:
            pool = new hwRegPool;
            break;
        case 3:
            pool = new memRegPool;
            break;
        default:
            func_0804ad38("Processor::Init - bad type_id found\n");
            return false;
        }
        if (!pool)
            func_0804ad38("Processor::Init  - failed to get a regpool '%s'\n", d->name);
        if (!pool->Init(d->unk_04, d->name, d->cls, d->unk_10)) {
            func_0804ad38("failed to init reg pool '%s'\n", d->name);
            delete pool;
            return false;
        }
        if (d->zeroReg) {
            RegPoolEntry *e = pool->func_08059418(d->zeroReg);
            if (!e) {
                func_0804ad38("failed to get zero reg for pool %s\n", d->name);
                delete pool;
                return false;
            }
            pool->unk_10 = e;
        }
        if (d->extReg) {
            RegPoolEntry *e = pool->func_08059418(d->extReg);
            if (!e) {
                func_0804ad38("failed to get ext reg for pool %s\n", d->name);
                delete pool;
                return false;
            }
            pool->unk_14 = e;
        }
        if (g_080de6e0[d->cls]) {
            func_0804ad38("already initialised reg type %d\n", d->cls);
            delete pool;
            return false;
        }
        g_080de6e0[d->cls] = pool;
    }
    if (!cpu->dstSpecs) {
        func_0804ad38("procDec has no dstSpecs loaded\n");
        return false;
    }
    g_080de728 = cpu->dstSpecs;
    if (!cpu->srcSpecs) {
        func_0804ad38("procDec has no srcSpecs loaded\n");
        return false;
    }
    g_080de724 = cpu->srcSpecs;
    return true;
}

bool Processor::func_0806bc84()
{
    unk_4c.func_0804b1b0();
    unk_20.func_0804b1b0();
    unk_04.func_0804b1b0();
    return true;
}

void Processor::func_0806bcb8()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i])
            g_080de6e0[i]->Dump();
}

int Processor::gatherInflight(ol_s **out)
{
    int n = 0;
    for (int i = 0; i != 12; i++) {
        if (g_080de6e0[i]) {
            int r = g_080de6e0[i]->gatherInflight(out + n);
            if (r < 0) {
                func_0804ad38("error during inflight gather\n");
                return -1;
            }
            n += r;
        }
    }
    return n;
}

bool Processor::func_0806bd40()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059c4c()) {
            func_0804ad38("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

bool Processor::func_0806bd88()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059c94()) {
            func_0804ad38("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

void Processor::func_0806bdd0()
{
    if (!g_080de6c0) {
        func_0804ad38("iInfos not set in processor\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    int n = 0;
    for (iInfo *p = g_080de6c0; p->unk_04; p++) {
        p->func_08070080();
        n++;
    }
    if (n != 0xd6) {
        func_0804ad38("error processing inst table\n");
        func_0804adf0(-1);
    }
    errorOccurred = saved;
}
