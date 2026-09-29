// prune pass helpers; inline, emitted out-of-line via #pragma implementation in prune.cpp.
#ifndef PRUNE_H
#define PRUNE_H

#pragma interface

#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

inline bool initPrune(instStream *s);
inline bool initPrune(Instr *in);
inline int tryPrune(instStream *s);
int tryPrune(Instr *in, bool raw);
bool doPrune(instStream *s);

inline bool initPrune(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108)
        if (!initPrune(s)) {
            func_0804ad38("failed to init prune for %s\n", s->func_0806d674());
            return false;
        }
    return true;
}

inline bool initPrune(instStream *s)
{
    workTable *t = s->getTable();
    if (!t) {
        func_0804ad38("failed to get work table\n");
        return false;
    }
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        if (!initPrune(in)) {
            func_0804ad38("init prune failed\n");
            return false;
        }
    return true;
}

inline bool initPrune(Instr *in)
{
    in->unk_13cc = 0;
    if (in->unk_34) {
        if (!in->unk_34->func_0807026c())
            in->unk_13cc = 1;
        for (int i = 0;; i++) {
            Operand *op = in->getOp(i);
            if (!op)
                break;
            op->unk_2a = 0;
            if (op->unk_09 != 1 || !in->unk_13cc)
                continue;
            op->unk_2a = op->unk_07;
        }
        return true;
    }
    func_0804ad38("found inst with no m_Inst\n");
    return false;
}

inline int tryPrune(Program *p)
{
    int n = 0;
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        int r = tryPrune(s);
        if (r < 0) {
            func_0804ad38("flowMon: failed to  tryPrune %s\n", s->func_0806d674());
            return -1;
        }
        n += r;
    }
    return n;
}

inline int tryPrune(instStream *s)
{
    int n = 0;
    workTable *t = s->getTable();
    if (!t) {
        func_0804ad38("failed to get work table\n");
        return 0;
    }
    bool raw = false;
    if (t->unk_09 == 4 || t->func_08073754() == 4)
        raw = true;
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int r = tryPrune(in, raw);
        if (r < 0) {
            func_0804ad38("rec  prune failed for instruction\n");
            in->func_08054008(false);
            n = -1;
            break;
        }
        n += r;
    }
    return n;
}

inline bool doPrune(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108)
        if (!doPrune(s)) {
            func_0804ad38("failed to do prune for %s\n", s->func_0806d674());
            return false;
        }
    return true;
}

#endif
