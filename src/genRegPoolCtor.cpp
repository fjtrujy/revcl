#include "os.h"
#include "genRegPool_shared.h"

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void func_0804c220(char *p, int n);
RegPool *GetRP(unsigned t);
bool func_0806f610(Program *p);
bool func_0806f6a4(Program *p);
bool setRange(int start, int end, int lsz);
int func_0804bf50(unsigned int);
int setRangeFrom(workTable *t);


genRegPool::genRegPool()
{
    unk_0e = 1;
}

genRegPool::~genRegPool()
{
}

bool genRegPool::func_08088e54(workTable *t)
{
    if (!t) {
        reportError("failed to get itable in nassign - which is not really likely\n");
        return false;
    }
    if (!func_08059cdc()) {
        reportError("failed to reset local allocation scheme for pool\n");
        return false;
    }
    setRangeFrom(t);
    if (!initAlloc(t->func_0807411c(), t->func_080741c0())) {
        reportError("failed to set table range\n");
        return false;
    }
    regUse *ru = t->unk_14.firstReal();
    regUseList *list = &t->unk_14;
    for (; ru; ru = ru->nextReal()) {
        if (!set_in_aTab(ru)) {
            reportError("failed to set forced register in aTab\n");
            t->dump();
            list->func_0805b44c();
            exitAfterCleanup(-1);
            return false;
        }
    }
    RegPoolEntry *todo = 0;
    Instr *in;
    for (in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->unk_18;
            if (func_08059e20(reg) && reg->unk_14 && reg->unk_3c < 0) {
                if (!build_tTab(reg, (1 << reg->partCount) - 1)) {
                    reportError("failed to build ttab while building todolist\n");
                    t->dump();
                    exitAfterCleanup(-1);
                }
                reg->unk_3c = countTTab();
                reg->unk_40 = todo;
                todo = reg;
                if (reg->unk_3c < 0) {
                    reportError("-ve ttabsize\n");
                    exitAfterCleanup(-1);
                }
            }
        }
    }
    while (todo) {
        int max = -1;
        RegPoolEntry *best = 0;
        RegPoolEntry *bestPrev = 0;
        RegPoolEntry *prev = 0;
        for (RegPoolEntry *p = todo; p; prev = p, p = p->unk_40) {
            if (p->unk_3c > max) {
                best = p;
                bestPrev = prev;
                max = p->unk_3c;
            }
        }
        if (!best) {
            reportError("failed to find lowest register!\n");
            exitAfterCleanup(-1);
        }
        if (!build_tTab(best, (1 << best->partCount) - 1)) {
            reportError("failed to build tta externalb\n");
            exitAfterCleanup(-1);
        }
        int r = bestReg();
        if (r < 0) {
            if (!func_08059f70()) {
                reportError("vcl failed to allocate a register of type %s, but there's no registers of this type available for it to use.. maybe adding a .init_.. directive would help\n", unk_2c);
                func_0804ac44();
                return false;
            } else
                list->func_0805bab0();
            return false;
        }
        best->unk_18 = &unk_04[r];
        if (!set_t_in_aTab(r)) {
            ru->func_0805b51c();
            list->func_0805b44c();
            reportError("failed to set t\n");
            exitAfterCleanup(-1);
        }
        dumpATab();
        dumpTTab();
        if (bestPrev)
            bestPrev->unk_40 = best->unk_40;
        else
            todo = best->unk_40;
    }
    for (in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->unk_18;
            if (func_08059e20(reg) && reg->unk_14) {
                if (!reg->unk_18) {
                    reportError("notmapped!, should have been mapped by now..%s!\n",
                                  reg->registerName());
                    exitAfterCleanup(-1);
                }
                op->unk_18 = reg->unk_18;
            }
        }
    }
    dumpATab();
    return true;
}

bool genRegPool::assignExt(instStream *s)
{
    if (!s) {
        reportError("AssignExternalRegisters - passed a null  pointer!\n");
        return false;
    }
    Program *owner = s->unk_104;
    if (!owner) {
        reportError("AssignExternalRegisters - stream had no owner\n");
        return false;
    }
    workTable *wt = s->getTable();
    for (regUse *ru = wt->unk_14.firstVirtual(); ru; ru = ru->nextVirtual()) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            reportError("flowMon::AssignExt: register not set in rf\n");
            return false;
        }
        if (func_08059e20(reg) && ru->isExternal() && !reg->unk_18) {
            initRegMask(this);
            for (instStream *p = owner->streamHead; p; p = p->unk_108) {
                if (!func_08088e6c(p, reg)) {
                    reportError("generic external allocator had internal error.. ret<0 for DiscardDisjoint\n");
                    return false;
                }
            }
            int r = firstEmptyMask(this);
            if (r < 0) {
                if (!func_08059f70()) {
                    reportError("vcl failed to assign a register of type %s, but there's no registers of that type available for it to use, maybe adding a .init.. directive would help\n");
                    func_0804ac44();
                    return false;
                }
                reportInfo("currently at stream: %s trying to allocate external register %s\n",
                              s->streamName(), reg->registerName());
                wt->unk_14.func_0805bab0();
                reportError("failed to assign an external %s\n", reg->registerName());
                func_0804ac44();
                return false;
            }
            reg->unk_18 = &unk_04[r];
        }
    }
    return true;
}

bool genRegPool::func_08088e6c(instStream *s, RegPoolEntry *reg)
{
    if (!reg || !func_08059e20(reg)) {
        reportError("discard disjoint called, but this pool doesn't own this register!\n");
        return false;
    }
    if (reg->unk_18) {
        reportError("rg_a is mapped.. at start of DiscardDisjoint registers %s\n",
                      reg->registerName());
        return false;
    }
    workTable *wt = s->getTable();
    regUseList *l = &wt->unk_14;
    regUse *ru;
    for (ru = l->firstVirtual(); ru; ru = ru->nextVirtual()) {
        if (ru->unk_08 == reg) {
            if (!ru->isExternal()) {
                reportError("discard disjoint - found a match - but it's not external\n");
                return false;
            }
            break;
        }
    }
    if (!ru)
        return true;
    if (!(ru->unk_0c | ru->unk_0d | ru->unk_10)) {
        reportError("genRegPool: discard disjoint - an xnf register had was not xixot\n");
        return false;
    }
    if (ru->unk_0c & ~ru->unk_0e) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    if (ru->unk_0d & ~ru->unk_0f) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    for (regUse *o = l->unk_04; o; o = o->unk_04) {
        RegPoolEntry *r = o->unk_08;
        if (!r) {
            reportError("flowMon::discard disjoint forced regref with null reg\n");
            return false;
        }
        if (!func_08059e20(r) || (!r->unk_18 && r->unk_14))
            continue;
        unsigned char m = 0;
        switch (wt->unk_09) {
        case 1:
        case 4: {
            unsigned char a = ru->unk_0e | ru->unk_0f;
            if (ru->unk_0f)
                a |= 0x80;
            unsigned char b = o->unk_0e | o->unk_0f;
            if (o->unk_0f)
                b |= 0x80;
            m = b & a;
            m |= (a & o->unk_10) | (b & ru->unk_10);
            break;
        }
        case 3:
            for (int i = 0; i != reg->partCount; i++) {
                int as = 0, ae = 0;
                bool aa = false;
                int bs = 0, be = 0;
                bool ba = false;
                if (!regRefLiveRange(ru, i, &as, &ae, &aa)) {
                    reportError("couldn't determine live range_a\n");
                    return false;
                }
                if (!regRefLiveRange(o, i, &bs, &be, &ba)) {
                    reportError("couldn't determine live range_b.%d\n", i);
                    o->func_0805b51c();
                    wt->dump();
                    return false;
                }
                if (!(aa && ba) && (be >= 0 || ae >= 0) && as < be && ae > bs)
                    m |= 1 << i;
            }
            if (ru->unk_0f && o->unk_0f && hack_slo(wt, reg, r))
                m |= 0x80;
            break;
        default:
            return false;
        }
        RegPoolEntry *tmpr;
        if (r->unk_14 != 1)
            tmpr = r;
        else
            tmpr = r->unk_18;
        if (!tmpr) {
            reportError("tmpr is zero\n");
            return false;
        }
        if (tmpr->unk_14) {
            reportError("tmpr is virtual\n");
            return false;
        }
        regMask_080dde40[tmpr->regIndex()] |= m;
    }
    return true;
}
