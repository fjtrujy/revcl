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
    if (!resetPoolLocalAllocationState()) {
        reportError("failed to reset local allocation scheme for pool\n");
        return false;
    }
    setRangeFrom(t);
    if (!initAlloc(t->getRangeStart(), t->getRangeEnd())) {
        reportError("failed to set table range\n");
        return false;
    }
    regUse *ru = t->registerUses.firstReal();
    regUseList *list = &t->registerUses;
    for (; ru; ru = ru->nextReal()) {
        if (!set_in_aTab(ru)) {
            reportError("failed to set forced register in aTab\n");
            t->dump();
            list->dumpRegisterUses();
            exitAfterCleanup(-1);
            return false;
        }
    }
    RegPoolEntry *todo = 0;
    Instr *in;
    for (in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->assignedRegister;
            if (ownsRegisterClass(reg) && reg->isVirtual && reg->assignmentCostTableSize < 0) {
                if (!build_tTab(reg, (1 << reg->partCount) - 1)) {
                    reportError("failed to build ttab while building todolist\n");
                    t->dump();
                    exitAfterCleanup(-1);
                }
                reg->assignmentCostTableSize = countTTab();
                reg->nextPendingAssignment = todo;
                todo = reg;
                if (reg->assignmentCostTableSize < 0) {
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
        for (RegPoolEntry *p = todo; p; prev = p, p = p->nextPendingAssignment) {
            if (p->assignmentCostTableSize > max) {
                best = p;
                bestPrev = prev;
                max = p->assignmentCostTableSize;
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
            if (!countMarkedPhysicalRegisters()) {
                reportError("vcl failed to allocate a register of type %s, but there's no registers of this type available for it to use.. maybe adding a .init_.. directive would help\n", poolName);
                func_0804ac44();
                return false;
            } else
                list->dumpVirtualRegisterUses();
            return false;
        }
        best->assignedPhysicalRegister = &registerEntries[r];
        if (!set_t_in_aTab(r)) {
            ru->validateAndDumpParts();
            list->dumpRegisterUses();
            reportError("failed to set t\n");
            exitAfterCleanup(-1);
        }
        dumpATab();
        dumpTTab();
        if (bestPrev)
            bestPrev->nextPendingAssignment = best->nextPendingAssignment;
        else
            todo = best->nextPendingAssignment;
    }
    for (in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->assignedRegister;
            if (ownsRegisterClass(reg) && reg->isVirtual) {
                if (!reg->assignedPhysicalRegister) {
                    reportError("notmapped!, should have been mapped by now..%s!\n",
                                  reg->registerName());
                    exitAfterCleanup(-1);
                }
                op->assignedRegister = reg->assignedPhysicalRegister;
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
    Program *owner = s->owningProgram;
    if (!owner) {
        reportError("AssignExternalRegisters - stream had no owner\n");
        return false;
    }
    workTable *wt = s->getTable();
    for (regUse *ru = wt->registerUses.firstVirtual(); ru; ru = ru->nextVirtual()) {
        RegPoolEntry *reg = ru->registerEntry;
        if (!reg) {
            reportError("flowMon::AssignExt: register not set in rf\n");
            return false;
        }
        if (ownsRegisterClass(reg) && ru->isExternal() && !reg->assignedPhysicalRegister) {
            initRegMask(this);
            for (instStream *p = owner->streamHead; p; p = p->nextStream) {
                if (!func_08088e6c(p, reg)) {
                    reportError("generic external allocator had internal error.. ret<0 for DiscardDisjoint\n");
                    return false;
                }
            }
            int r = firstEmptyMask(this);
            if (r < 0) {
                if (!countMarkedPhysicalRegisters()) {
                    reportError("vcl failed to assign a register of type %s, but there's no registers of that type available for it to use, maybe adding a .init.. directive would help\n");
                    func_0804ac44();
                    return false;
                }
                reportInfo("currently at stream: %s trying to allocate external register %s\n",
                              s->streamName(), reg->registerName());
                wt->registerUses.dumpVirtualRegisterUses();
                reportError("failed to assign an external %s\n", reg->registerName());
                func_0804ac44();
                return false;
            }
            reg->assignedPhysicalRegister = &registerEntries[r];
        }
    }
    return true;
}

bool genRegPool::func_08088e6c(instStream *s, RegPoolEntry *reg)
{
    if (!reg || !ownsRegisterClass(reg)) {
        reportError("discard disjoint called, but this pool doesn't own this register!\n");
        return false;
    }
    if (reg->assignedPhysicalRegister) {
        reportError("rg_a is mapped.. at start of DiscardDisjoint registers %s\n",
                      reg->registerName());
        return false;
    }
    workTable *wt = s->getTable();
    regUseList *l = &wt->registerUses;
    regUse *ru;
    for (ru = l->firstVirtual(); ru; ru = ru->nextVirtual()) {
        if (ru->registerEntry == reg) {
            if (!ru->isExternal()) {
                reportError("discard disjoint - found a match - but it's not external\n");
                return false;
            }
            break;
        }
    }
    if (!ru)
        return true;
    if (!(ru->externalInputMask | ru->externalOutputMask | ru->throughputMask)) {
        reportError("genRegPool: discard disjoint - an xnf register had was not xixot\n");
        return false;
    }
    if (ru->externalInputMask & ~ru->inputMask) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    if (ru->externalOutputMask & ~ru->outputMask) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    for (regUse *o = l->head; o; o = o->next) {
        RegPoolEntry *r = o->registerEntry;
        if (!r) {
            reportError("flowMon::discard disjoint forced regref with null reg\n");
            return false;
        }
        if (!ownsRegisterClass(r) || (!r->assignedPhysicalRegister && r->isVirtual))
            continue;
        unsigned char m = 0;
        switch (wt->codeModeValue) {
        case 1:
        case 4: {
            unsigned char a = ru->inputMask | ru->outputMask;
            if (ru->outputMask)
                a |= 0x80;
            unsigned char b = o->inputMask | o->outputMask;
            if (o->outputMask)
                b |= 0x80;
            m = b & a;
            m |= (a & o->throughputMask) | (b & ru->throughputMask);
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
                    o->validateAndDumpParts();
                    wt->dump();
                    return false;
                }
                if (!(aa && ba) && (be >= 0 || ae >= 0) && as < be && ae > bs)
                    m |= 1 << i;
            }
            if (ru->outputMask && o->outputMask && hack_slo(wt, reg, r))
                m |= 0x80;
            break;
        default:
            return false;
        }
        RegPoolEntry *tmpr;
        if (r->isVirtual != 1)
            tmpr = r;
        else
            tmpr = r->assignedPhysicalRegister;
        if (!tmpr) {
            reportError("tmpr is zero\n");
            return false;
        }
        if (tmpr->isVirtual) {
            reportError("tmpr is virtual\n");
            return false;
        }
        regMask_080dde40[tmpr->regIndex()] |= m;
    }
    return true;
}
