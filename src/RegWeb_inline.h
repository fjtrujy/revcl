// Deferred inline helpers of RegWeb; emitted out of line via #pragma implementation in RegWeb.cpp.
#ifndef REGWEB_INLINE_H
#define REGWEB_INLINE_H

#pragma interface

#include "RegWeb.h"
#include "Program.h"
#include "instStream.h"

bool func_0806c6dc(Program *p);

inline bool resetRegWebs()
{
    regWebCount = 0;
    regWebUsed = 0;
    return true;
}

inline bool RegWeb::AddiTable(workTable *t)
{
    if (!t) {
        func_0804ad38("RegWeb::AddiTable - passed a null pointer\n");
        return false;
    }
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (!AddiInstr(in)) {
            func_0804ad38("RegWeb::AddiTable: failed to add an isn\n");
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
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *vrg = op->unk_18;
        if (!vrg || !op->isForwardable())
            continue;
        RegWeb *w = op->unk_00;
        if (!op->GetWeb()) {
            w = GetNewWeb(vrg);
            if (!w) {
                func_0804ad38("failed to get a web entry for forwardable operand\n");
                return false;
            }
            op->unk_00 = w;
        }
        w->unk_14 |= op->unk_07;
        if (op->unk_0d && op->unk_09 == 1) {
            if (!merge(w, in->unk_38[op->unk_0d - 1].GetWeb())) {
                func_0804ad38("failed to merge web for implicit web patch\n");
                return false;
            }
        }
        if (op->unk_10) {
            RegPoolEntry *fb = op->unk_10;
            if (fb->unk_14) {
                func_0804ad38("can't have a virtual forced register\n");
                return false;
            }
            if (!setfMap(w, fb)) {
                func_0804ad38("setfMap failed when binding to base for basereg %s, reg %s\n",
                              fb->func_0805a8e0(), vrg->func_0805a8e0());
                return false;
            }
            if (!vrg->unk_14 && vrg != fb) {
                func_0804ad38("register isn't virtual - but doesn't match base for forced base reg\n");
                return false;
            }
        }
        if (isRegisterOperand(op, 1)) {
            for (int j = 0; j != vrg->unk_28; j++) {
                bool used = (op->unk_07 >> j) & 1;
                if (!used)
                    continue;
                ol_s *ol = op->unk_2c[j].unk_54.unk_00;
                if (!ol) {
                    func_0804ad38("found input operand part with empty olList..\n");
                    func_0804ad38("this should have been caught by now..\n");
                    func_0804ad38("stopping\n");
                    in->leDump(i, j);
                    return false;
                }
                do {
                    Operand *src = func_0805802c(ol);
                    if (!src) {
                        func_0804ad38("getoploc - empty!\n");
                        in->leDump(i, j);
                        ol->func_08057fbc();
                    }
                    if (!src->GetWeb()) {
                        src->unk_00 = GetNewWeb(vrg);
                        if (!src->unk_00) {
                            func_0804ad38("failed to get a web entry for forwardable operand\n");
                            return false;
                        }
                    }
                    src->unk_00->unk_14 |= src->unk_07;
                    if (!merge(src->GetWeb(), w)) {
                        func_0804ad38("failed to merge web entries\n");
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
        func_0804ad38("oh no! ran out of regweb entries\n");
        return 0;
    }
    if (!old_vrg) {
        func_0804ad38("GetNewWeb  - old_vrg not set\n");
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
        func_0804ad38("RegWeb::merge.. w_a was null!\n");
        return false;
    }
    unsigned char fa = 0, fb = 0;
    while (w_a->unk_0c) {
        fa |= w_a->unk_14;
        if (w_a->unk_10) {
            func_0804ad38("Merge: inconsistency - nUsed on chase\n");
            func_0804adf0(-1);
            return false;
        }
        if (w_a->unk_08) {
            func_0804ad38("non-terminal web a had mapping\n");
            func_0804adf0(-1);
            return false;
        }
        w_a = w_a->unk_0c;
    }
    if (!w_a->unk_10) {
        func_0804ad38("hit a terminal empty strand in web\n");
        return false;
    }
    if (!w_b) {
        func_0804ad38("RegWeb::merge .. b was null\n");
        return false;
    }
    while (w_b->unk_0c) {
        fb |= w_b->unk_14;
        if (w_b->unk_10) {
            func_0804ad38("Merge: inconsistency - nUsed on chase for w_b\n");
            return false;
        }
        if (w_b->unk_08) {
            func_0804ad38("web b had mapping\n");
            func_0804adf0(-1);
            return false;
        }
        w_b = w_b->unk_0c;
    }
    if (!w_b->unk_10) {
        func_0804ad38("hit a terminal empty strand in web\n");
        return false;
    }
    if (w_a == w_b)
        return true;
    if (w_b->unk_00 || w_a->unk_00) {
        func_0804ad38("one or both strands had register attached!\n");
        return false;
    }
    unsigned char m = fa | fb | w_b->unk_14 | w_a->unk_14;
    w_a->unk_14 = m;
    w_b->unk_14 = m;
    if (w_a->unk_08) {
        if (!setfMap(w_b, w_a->unk_08)) {
            func_0804ad38("failed to load mapping from a into b\n");
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
    RegWeb *w = unk_2b4;
    if (!w)
        return 0;
    while (w->unk_0c) {
        if (w->unk_10) {
            func_0804ad38(" INTERNAL Register::GetWeb found non terminal count not at end!! for '%s' count=%d\n",
                          func_0805a8e0(), w->unk_10);
            func_0804adf0(-1);
        }
        w = w->unk_0c;
    }
    if (!w->unk_10) {
        func_0804ad38("INTERNAL Register::terminal entry had zero count!\n");
        func_0804adf0(-1);
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
            func_0804ad38("INTERNAL : Operand::GetWeb found non terminal count!\n");
            func_0804adf0(-1);
        }
        w = w->unk_0c;
    }
    if (!w->unk_10) {
        func_0804ad38("INTERNAL : Operand::terminal entry had zero count!\n");
        func_0804adf0(-1);
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
            if (r->unk_14 == 1) {
                func_0804ad38("forced to a non-base register\n");
                return false;
            }
            if (r->unk_2b4) {
                if (!RegWeb::merge(r->unk_2b4, w)) {
                    func_0804ad38("during mapped merge..failed for %s\n", r->func_0805a8e0());
                    return false;
                }
            } else
                r->unk_2b4 = w;
        }
    }
    for (i = 0; i != regWebCount; i++) {
        RegWeb *w = &regWebs[i];
        if (w->unk_10) {
            RegPoolEntry *r = w->unk_04;
            if (!r) {
                func_0804ad38("GatherWeb:: oh no - dReg not set\n");
                func_0804adf0(-1);
                return false;
            }
            if (w->unk_00) {
                func_0804ad38("found register assign at unexpected place!\n");
                func_0804adf0(-1);
            }
            if (w->unk_08)
                w->unk_00 = w->unk_08;
            else {
                RegPool *pool = r->unk_04;
                if (!pool) {
                    func_0804ad38("Gather: register had no pool\n");
                    return false;
                }
                func_0804adbc(name, "%s__%d", r->func_0805a8e0(), r->unk_10);
                w->unk_00 = pool->func_08059a5c(name);
                if (!w->unk_00) {
                    func_0804ad38("GatherWeb: failed to get '%s' from %s\n", name, pool->unk_2c);
                    return false;
                }
                w->unk_00->unk_2b4 = w;
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
            func_0804ad38("patch operand - web has no reg\n");
            return false;
        }
        RegPoolEntry *r = w->unk_00;
        op->unk_18 = r;
        if (op->unk_10 && op->unk_10 != r) {
            func_0804ad38("not forcing to a base reg after all - in PatchOperands %s isn't %s\n",
                          op->unk_10->func_0805a8e0(), r->func_0805a8e0());
            int have = op->unk_18->unk_14;
            int want = op->unk_10->unk_14;
            func_0804ad38("have %d %d\n", want, have);
            return false;
        }
    }
    return true;
}

inline bool RegWeb::PatchInstr(Instr *in)
{
    if (!in)
        return true;
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        if (!PatchOperand(op))
            return false;
    }
    return true;
}

inline bool RegWeb::PatchITable(workTable *t)
{
    if (t) {
        for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            if (!PatchInstr(in)) {
                func_0804ad38("patchiTable - failed\n");
                in->func_08054008(false);
                goto fail;
            }
        }
        return true;
    }
    func_0804ad38("RegWeb::PatchITable - passed a null pointer\n");
    return false;
fail:
    return false;
}

inline void RegWeb::Dump()
{
    for (int i = 0; i != regWebCount; i++) {
        if (regWebs[i].unk_10 && !regWebs[i].unk_04) {
            func_0804ad38("Dump: dReg not set\n");
            func_0804adf0(-1);
        }
    }
}

inline bool RegWeb::setfMap(RegWeb *w, RegPoolEntry *r)
{
    while (w->unk_0c)
        w = w->unk_0c;
    if (!r) {
        func_0804ad38("regweb::setfMap - asked to set a null mapping\n");
        func_0804adf0(-1);
        return false;
    }
    if (r->unk_14) {
        func_0804ad38("regweb:: setfMap - can't setfmap to virtual!\n");
        func_0804adf0(-1);
        return false;
    }
    if (w->unk_08) {
        if (w->unk_08 != r) {
            func_0804ad38("regweb:: set map - can't map to different base registers..\n");
            func_0804ad38("regweb:: try to map both %s and %s\n",
                          w->unk_08->func_0805a8e0(), r->func_0805a8e0());
            return false;
        }
    } else
        w->unk_08 = r;
    return true;
}

inline bool buildRegWebs(Program *p)
{
    resetRegWebs();
    if (!func_0806c6dc(p)) {
        func_0804ad38("flow:: reset the webs\n");
        return false;
    }
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108)
        RegWeb::ClearWebs(s->getWorkTable());
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!RegWeb::AddiTable(s->getWorkTable())) {
            func_0804ad38("flow: AddIStream failed for %s\n", s->func_0806d674());
            return false;
        }
    }
    if (!GatherWeb()) {
        func_0804ad38("flow::GatherWeb() failed\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!RegWeb::PatchITable(s->getWorkTable())) {
            func_0804ad38("flow: failed to patch ist %s\n", s->func_0806d674());
            return false;
        }
    }
    return true;
}

inline bool RegWeb::ClearWebs(workTable *t)
{
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        for (int i = 0; i != in->unk_138b; i++)
            in->unk_38[i].unk_00 = 0;
    return true;
}

#endif
