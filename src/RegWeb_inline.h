// Deferred inline helpers of RegWeb; emitted out of line via #pragma implementation in RegWeb.cpp.
#ifndef REGWEB_INLINE_H
#define REGWEB_INLINE_H

#pragma interface

#include "RegWeb.h"
#include "Program.h"
#include "instStream.h"

bool resetPoolOptimizationState(Program *p);

inline bool resetRegWebs()
{
    regWebCount = 0;
    regWebUsed = 0;
    return true;
}

inline bool RegWeb::AddiTable(workTable *t)
{
    if (!t) {
        reportError("RegWeb::AddiTable - passed a null pointer\n");
        return false;
    }
    for (Instr *in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        if (!AddiInstr(in)) {
            reportError("RegWeb::AddiTable: failed to add an isn\n");
            in->func_08054008(false);
            t->dump();
            return false;
        }
    }
    return true;
}

inline bool RegWeb::AddiInstr(Instr *in)
{
    if (!in)
        return false;
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *vrg = op->assignedRegister;
        if (!vrg || !op->isForwardable())
            continue;
        RegWeb *w = op->registerWeb;
        if (!op->GetWeb()) {
            w = GetNewWeb(vrg);
            if (!w) {
                reportError("failed to get a web entry for forwardable operand\n");
                return false;
            }
            op->registerWeb = w;
        }
        w->componentMask |= op->componentMask;
        if (op->pairedOperandIndexPlusOne && op->direction == 1) {
            if (!merge(w, in->operands[op->pairedOperandIndexPlusOne - 1].GetWeb())) {
                reportError("failed to merge web for implicit web patch\n");
                return false;
            }
        }
        if (op->forcedBaseRegister) {
            RegPoolEntry *fb = op->forcedBaseRegister;
            if (fb->isVirtual) {
                reportError("can't have a virtual forced register\n");
                return false;
            }
            if (!setfMap(w, fb)) {
                reportError("setfMap failed when binding to base for basereg %s, reg %s\n",
                              fb->registerName(), vrg->registerName());
                return false;
            }
            if (!vrg->isVirtual && vrg != fb) {
                reportError("register isn't virtual - but doesn't match base for forced base reg\n");
                return false;
            }
        }
        if (isRegisterOperand(op, 1)) {
            for (int j = 0; j != vrg->partCount; j++) {
                bool used = (op->componentMask >> j) & 1;
                if (!used)
                    continue;
                ol_s *ol = op->parts[j].unk_54.head;
                if (!ol) {
                    reportError("found input operand part with empty olList..\n");
                    reportError("this should have been caught by now..\n");
                    reportError("stopping\n");
                    in->leDump(i, j);
                    return false;
                }
                do {
                    Operand *src = operandAtLocation(ol);
                    if (!src) {
                        reportError("getoploc - empty!\n");
                        in->leDump(i, j);
                        ol->dumpInstructionContext();
                    }
                    if (!src->GetWeb()) {
                        src->registerWeb = GetNewWeb(vrg);
                        if (!src->registerWeb) {
                            reportError("failed to get a web entry for forwardable operand\n");
                            return false;
                        }
                    }
                    src->registerWeb->componentMask |= src->componentMask;
                    if (!merge(src->GetWeb(), w)) {
                        reportError("failed to merge web entries\n");
                        return false;
                    }
                    ol = ol->next;
                } while (ol);
            }
        }
    }
    return true;
}

inline RegWeb *RegWeb::GetNewWeb(RegPoolEntry *old_vrg)
{
    if (regWebCount >= 0x4000) {
        reportError("oh no! ran out of regweb entries\n");
        return 0;
    }
    if (!old_vrg) {
        reportError("GetNewWeb  - old_vrg not set\n");
        return 0;
    }
    RegWeb *w = &regWebs[regWebCount];
    w->assignedRegister = 0;
    w->virtualRegister = old_vrg;
    w->componentMask = 0;
    w->parentWeb = 0;
    w->memberCount = 1;
    w->webIndex = regWebCount;
    w->forcedRegister = 0;
    regWebCount++;
    return w;
}

inline bool RegWeb::merge(RegWeb *w_a, RegWeb *w_b)
{
    if (!w_a) {
        reportError("RegWeb::merge.. w_a was null!\n");
        return false;
    }
    unsigned char fa = 0, fb = 0;
    while (w_a->parentWeb) {
        fa |= w_a->componentMask;
        if (w_a->memberCount) {
            reportError("Merge: inconsistency - nUsed on chase\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (w_a->forcedRegister) {
            reportError("non-terminal web a had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        w_a = w_a->parentWeb;
    }
    if (!w_a->memberCount) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (!w_b) {
        reportError("RegWeb::merge .. b was null\n");
        return false;
    }
    while (w_b->parentWeb) {
        fb |= w_b->componentMask;
        if (w_b->memberCount) {
            reportError("Merge: inconsistency - nUsed on chase for w_b\n");
            return false;
        }
        if (w_b->forcedRegister) {
            reportError("web b had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        w_b = w_b->parentWeb;
    }
    if (!w_b->memberCount) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (w_a == w_b)
        return true;
    if (w_b->assignedRegister || w_a->assignedRegister) {
        reportError("one or both strands had register attached!\n");
        return false;
    }
    unsigned char m = fa | fb | w_b->componentMask | w_a->componentMask;
    w_a->componentMask = m;
    w_b->componentMask = m;
    if (w_a->forcedRegister) {
        if (!setfMap(w_b, w_a->forcedRegister)) {
            reportError("failed to load mapping from a into b\n");
            return false;
        }
        w_a->forcedRegister = 0;
    }
    w_b->memberCount += w_a->memberCount;
    w_a->parentWeb = w_b;
    w_a->memberCount = 0;
    return true;
}

inline RegWeb *RegPoolEntry::GetWeb()
{
    RegWeb *w = registerWeb;
    if (!w)
        return 0;
    while (w->parentWeb) {
        if (w->memberCount) {
            reportError(" INTERNAL Register::GetWeb found non terminal count not at end!! for '%s' count=%d\n",
                          registerName(), w->memberCount);
            exitAfterCleanup(-1);
        }
        w = w->parentWeb;
    }
    if (!w->memberCount) {
        reportError("INTERNAL Register::terminal entry had zero count!\n");
        exitAfterCleanup(-1);
    }
    return w;
}

inline RegWeb *Operand::GetWeb()
{
    RegWeb *w = registerWeb;
    if (!w)
        return 0;
    while (w->parentWeb) {
        if (w->memberCount) {
            reportError("INTERNAL : Operand::GetWeb found non terminal count!\n");
            exitAfterCleanup(-1);
        }
        w = w->parentWeb;
    }
    if (!w->memberCount) {
        reportError("INTERNAL : Operand::terminal entry had zero count!\n");
        exitAfterCleanup(-1);
    }
    return w;
}

inline bool GatherWeb()
{
    int used = 0;
    char name[128];
    int i;
    for (i = 0; i != regWebCount; i++) {
        RegWeb *w = &regWebs[i];
        if (w->memberCount && w->forcedRegister) {
            RegPoolEntry *r = w->forcedRegister;
            if (r->isVirtual == 1) {
                reportError("forced to a non-base register\n");
                return false;
            }
            if (r->registerWeb) {
                if (!RegWeb::merge(r->registerWeb, w)) {
                    reportError("during mapped merge..failed for %s\n", r->registerName());
                    return false;
                }
            } else
                r->registerWeb = w;
        }
    }
    for (i = 0; i != regWebCount; i++) {
        RegWeb *w = &regWebs[i];
        if (w->memberCount) {
            RegPoolEntry *r = w->virtualRegister;
            if (!r) {
                reportError("GatherWeb:: oh no - dReg not set\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (w->assignedRegister) {
                reportError("found register assign at unexpected place!\n");
                exitAfterCleanup(-1);
            }
            if (w->forcedRegister)
                w->assignedRegister = w->forcedRegister;
            else {
                RegPool *pool = r->ownerPool;
                if (!pool) {
                    reportError("Gather: register had no pool\n");
                    return false;
                }
                formatIntoBuffer(name, "%s__%d", r->registerName(), r->uniqueNameSerial);
                w->assignedRegister = pool->createVirtualRegister(name);
                if (!w->assignedRegister) {
                    reportError("GatherWeb: failed to get '%s' from %s\n", name, pool->poolName);
                    return false;
                }
                w->assignedRegister->registerWeb = w;
                r->uniqueNameSerial++;
                used++;
            }
        }
    }
    regWebUsed = used;
    return true;
}

inline bool RegWeb::PatchOperand(Operand *op)
{
    if (!op)
        return true;
    RegWeb *w = op->GetWeb();
    if (w) {
        if (!w->assignedRegister) {
            reportError("patch operand - web has no reg\n");
            return false;
        }
        RegPoolEntry *r = w->assignedRegister;
        op->assignedRegister = r;
        if (op->forcedBaseRegister && op->forcedBaseRegister != r) {
            reportError("not forcing to a base reg after all - in PatchOperands %s isn't %s\n",
                          op->forcedBaseRegister->registerName(), r->registerName());
            int have = op->assignedRegister->isVirtual;
            int want = op->forcedBaseRegister->isVirtual;
            reportError("have %d %d\n", want, have);
            return false;
        }
    }
    return true;
}

inline bool RegWeb::PatchInstr(Instr *in)
{
    if (!in)
        return true;
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        if (!PatchOperand(op))
            return false;
    }
    return true;
}

inline bool RegWeb::PatchITable(workTable *t)
{
    if (t) {
        for (Instr *in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
            if (!PatchInstr(in)) {
                reportError("patchiTable - failed\n");
                in->func_08054008(false);
                goto fail;
            }
        }
        return true;
    }
    reportError("RegWeb::PatchITable - passed a null pointer\n");
    return false;
fail:
    return false;
}

inline void RegWeb::Dump()
{
    for (int i = 0; i != regWebCount; i++) {
        if (regWebs[i].memberCount && !regWebs[i].virtualRegister) {
            reportError("Dump: dReg not set\n");
            exitAfterCleanup(-1);
        }
    }
}

inline bool RegWeb::setfMap(RegWeb *w, RegPoolEntry *r)
{
    while (w->parentWeb)
        w = w->parentWeb;
    if (!r) {
        reportError("regweb::setfMap - asked to set a null mapping\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (r->isVirtual) {
        reportError("regweb:: setfMap - can't setfmap to virtual!\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (w->forcedRegister) {
        if (w->forcedRegister != r) {
            reportError("regweb:: set map - can't map to different base registers..\n");
            reportError("regweb:: try to map both %s and %s\n",
                          w->forcedRegister->registerName(), r->registerName());
            return false;
        }
    } else
        w->forcedRegister = r;
    return true;
}

inline bool buildRegWebs(Program *p)
{
    resetRegWebs();
    if (!resetPoolOptimizationState(p)) {
        reportError("flow:: reset the webs\n");
        return false;
    }
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream)
        RegWeb::ClearWebs(s->getWorkTable());
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!RegWeb::AddiTable(s->getWorkTable())) {
            reportError("flow: AddIStream failed for %s\n", s->streamName());
            return false;
        }
    }
    if (!GatherWeb()) {
        reportError("flow::GatherWeb() failed\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!RegWeb::PatchITable(s->getWorkTable())) {
            reportError("flow: failed to patch ist %s\n", s->streamName());
            return false;
        }
    }
    return true;
}

inline bool RegWeb::ClearWebs(workTable *t)
{
    for (Instr *in = t->linearInstructions.head; in; in = in->unk_1c.unk_00)
        for (int i = 0; i != in->operandCount; i++)
            in->operands[i].registerWeb = 0;
    return true;
}

#endif
