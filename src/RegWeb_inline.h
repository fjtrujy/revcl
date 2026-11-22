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
        RegPoolEntry *vrg = op->unk_18;
        if (!vrg || !op->isForwardable())
            continue;
        RegWeb *w = op->unk_00;
        if (!op->GetWeb()) {
            w = GetNewWeb(vrg);
            if (!w) {
                reportError("failed to get a web entry for forwardable operand\n");
                return false;
            }
            op->unk_00 = w;
        }
        w->unk_14 |= op->unk_07;
        if (op->unk_0d && op->unk_09 == 1) {
            if (!merge(w, in->operands[op->unk_0d - 1].GetWeb())) {
                reportError("failed to merge web for implicit web patch\n");
                return false;
            }
        }
        if (op->unk_10) {
            RegPoolEntry *fb = op->unk_10;
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
                bool used = (op->unk_07 >> j) & 1;
                if (!used)
                    continue;
                ol_s *ol = op->unk_2c[j].unk_54.head;
                if (!ol) {
                    reportError("found input operand part with empty olList..\n");
                    reportError("this should have been caught by now..\n");
                    reportError("stopping\n");
                    in->leDump(i, j);
                    return false;
                }
                do {
                    Operand *src = func_0805802c(ol);
                    if (!src) {
                        reportError("getoploc - empty!\n");
                        in->leDump(i, j);
                        ol->func_08057fbc();
                    }
                    if (!src->GetWeb()) {
                        src->unk_00 = GetNewWeb(vrg);
                        if (!src->unk_00) {
                            reportError("failed to get a web entry for forwardable operand\n");
                            return false;
                        }
                    }
                    src->unk_00->unk_14 |= src->unk_07;
                    if (!merge(src->GetWeb(), w)) {
                        reportError("failed to merge web entries\n");
                        return false;
                    }
                    ol = ol->unk_00;
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
    w->unk_00 = 0;
    w->unk_04 = old_vrg;
    w->unk_14 = 0;
    w->unk_0c = 0;
    w->unk_10 = 1;
    w->unk_12 = regWebCount;
    w->unk_08 = 0;
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
    while (w_a->unk_0c) {
        fa |= w_a->unk_14;
        if (w_a->unk_10) {
            reportError("Merge: inconsistency - nUsed on chase\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (w_a->unk_08) {
            reportError("non-terminal web a had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        w_a = w_a->unk_0c;
    }
    if (!w_a->unk_10) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (!w_b) {
        reportError("RegWeb::merge .. b was null\n");
        return false;
    }
    while (w_b->unk_0c) {
        fb |= w_b->unk_14;
        if (w_b->unk_10) {
            reportError("Merge: inconsistency - nUsed on chase for w_b\n");
            return false;
        }
        if (w_b->unk_08) {
            reportError("web b had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        w_b = w_b->unk_0c;
    }
    if (!w_b->unk_10) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (w_a == w_b)
        return true;
    if (w_b->unk_00 || w_a->unk_00) {
        reportError("one or both strands had register attached!\n");
        return false;
    }
    unsigned char m = fa | fb | w_b->unk_14 | w_a->unk_14;
    w_a->unk_14 = m;
    w_b->unk_14 = m;
    if (w_a->unk_08) {
        if (!setfMap(w_b, w_a->unk_08)) {
            reportError("failed to load mapping from a into b\n");
            return false;
        }
        w_a->unk_08 = 0;
    }
    w_b->unk_10 += w_a->unk_10;
    w_a->unk_0c = w_b;
    w_a->unk_10 = 0;
    return true;
}

inline RegWeb *RegPoolEntry::GetWeb()
{
    RegWeb *w = registerWeb;
    if (!w)
        return 0;
    while (w->unk_0c) {
        if (w->unk_10) {
            reportError(" INTERNAL Register::GetWeb found non terminal count not at end!! for '%s' count=%d\n",
                          registerName(), w->unk_10);
            exitAfterCleanup(-1);
        }
        w = w->unk_0c;
    }
    if (!w->unk_10) {
        reportError("INTERNAL Register::terminal entry had zero count!\n");
        exitAfterCleanup(-1);
    }
    return w;
}

inline RegWeb *Operand::GetWeb()
{
    RegWeb *w = unk_00;
    if (!w)
        return 0;
    while (w->unk_0c) {
        if (w->unk_10) {
            reportError("INTERNAL : Operand::GetWeb found non terminal count!\n");
            exitAfterCleanup(-1);
        }
        w = w->unk_0c;
    }
    if (!w->unk_10) {
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
        if (w->unk_10 && w->unk_08) {
            RegPoolEntry *r = w->unk_08;
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
        if (w->unk_10) {
            RegPoolEntry *r = w->unk_04;
            if (!r) {
                reportError("GatherWeb:: oh no - dReg not set\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (w->unk_00) {
                reportError("found register assign at unexpected place!\n");
                exitAfterCleanup(-1);
            }
            if (w->unk_08)
                w->unk_00 = w->unk_08;
            else {
                RegPool *pool = r->ownerPool;
                if (!pool) {
                    reportError("Gather: register had no pool\n");
                    return false;
                }
                formatIntoBuffer(name, "%s__%d", r->registerName(), r->unk_10);
                w->unk_00 = pool->createVirtualRegister(name);
                if (!w->unk_00) {
                    reportError("GatherWeb: failed to get '%s' from %s\n", name, pool->poolName);
                    return false;
                }
                w->unk_00->registerWeb = w;
                r->unk_10++;
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
        if (!w->unk_00) {
            reportError("patch operand - web has no reg\n");
            return false;
        }
        RegPoolEntry *r = w->unk_00;
        op->unk_18 = r;
        if (op->unk_10 && op->unk_10 != r) {
            reportError("not forcing to a base reg after all - in PatchOperands %s isn't %s\n",
                          op->unk_10->registerName(), r->registerName());
            int have = op->unk_18->isVirtual;
            int want = op->unk_10->isVirtual;
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
        if (regWebs[i].unk_10 && !regWebs[i].unk_04) {
            reportError("Dump: dReg not set\n");
            exitAfterCleanup(-1);
        }
    }
}

inline bool RegWeb::setfMap(RegWeb *w, RegPoolEntry *r)
{
    while (w->unk_0c)
        w = w->unk_0c;
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
    if (w->unk_08) {
        if (w->unk_08 != r) {
            reportError("regweb:: set map - can't map to different base registers..\n");
            reportError("regweb:: try to map both %s and %s\n",
                          w->unk_08->registerName(), r->registerName());
            return false;
        }
    } else
        w->unk_08 = r;
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
            in->operands[i].unk_00 = 0;
    return true;
}

#endif
