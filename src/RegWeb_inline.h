// Deferred inline helpers of RegWeb; emitted out of line via #pragma implementation in RegWeb.cpp.
#ifndef REGWEB_INLINE_H
#define REGWEB_INLINE_H

#pragma interface

#include "RegWeb.h"

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
        RegPoolEntry *fb = op->unk_10;
        if (fb) {
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
        if (op->isInRegOp()) {
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

#endif
