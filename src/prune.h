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
inline int tryPrune(Instr *in, bool raw);
inline bool doPrune(instStream *s);
inline int doPrune(Instr *in);
int hackIOmask(Instr *in, int outOp, unsigned char mask, int inOp);

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

inline int tryPrune(Instr *in, bool raw)
{
    int n = 0;
    if (!in->unk_34)
        return 0;
    if (raw) {
        for (int i = 0;; i++) {
            Operand *op = in->getOp(i);
            if (!op)
                break;
            RegPoolEntry *r = 0;
            if (op->unk_09 == 1)
                r = op->unk_18;
            if (r)
                op->unk_2a = op->unk_07;
        }
    }
    Operand *out;
    for (int o = 0; (out = in->getOp(o)) != 0; o++) {
        RegPoolEntry *reg = out->unk_09 == 2 ? out->unk_18 : 0;
        if (!reg)
            continue;
        int cnt = 0;
        for (int s = 0; s != reg->unk_28; s++) {
            if ((out->unk_07 >> s) & 1)
            for (ol_s *ol = out->unk_2c[s].unk_54.unk_00; ol; ol = ol->unk_00) {
                Operand *src = func_0805802c(ol);
                if (!src) {
                    func_0804ad38("failed to get an operand from an oploc\n");
                    return -1;
                }
                if (src->unk_18 != reg) {
                    func_0804ad38("there was a reg mismatch\n");
                    return -1;
                }
                if (ol->unk_11 != s) {
                    func_0804ad38("swizzle detected\n");
                    return -1;
                }
                if (!((src->unk_07 >> s) & 1)) {
                    func_0804ad38("there was a mop bit mismatch\n");
                    return -1;
                }
                if (((src->unk_2a >> s) & 1) && ((~out->unk_2a >> s) & 1)) {
                    cnt++;
                    n++;
                    out->unk_2a |= 1 << s;
                }
            }
        }
        if (!cnt)
            continue;
        if (out->unk_04 == 10 || out->unk_04 == 11)
            out->unk_2a = out->unk_07;
        Operand *ip;
        for (int k = 0; (ip = in->getOp(k)) != 0; k++) {
            RegPoolEntry *r = ip->unk_09 == 1 ? ip->unk_18 : 0;
            if (!r)
                continue;
            int bits = hackIOmask(in, o, out->unk_2a, k);
            if (bits < 0) {
                func_0804ad38("lprune: failed to deduce which inputs affected which outputs\n");
                return -1;
            }
            if ((ip->unk_07 & bits) != bits) {
                func_0804ad38("bits were used.. but outside of mod bit mask!\n");
                func_0804ad38("OUTPUT(%d) INPUT(%d) inst : \n", o, k);
                func_0804ad38("A_BITS=%x .. INPUT_MODBITS=%x : \n", bits, ip->unk_07);
                func_0804ad38("otype is opt->m_oType=%d\n", ip->unk_04);
                in->func_08054008(false);
                return -1;
            }
            ip->unk_2a |= bits;
        }
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

extern int errorOccurred;
extern bool quiet_080bbd98;

inline bool doPrune(instStream *s)
{
    workTable *t = s->getTable();
    if (!t) {
        func_0804ad38("failed to get work table\n");
        return false;
    }
    if (t->func_08073754() == 4 || t->func_08073754() == 3)
        return true;
    Instr *next;
    for (Instr *in = t->unk_88.unk_00; in; in = next) {
        next = in->unk_1c.unk_00;
        int r = doPrune(in);
        if (r < 0) {
            func_0804ad38("do  prune failed for instruction\n");
            in->func_08054008(false);
            return false;
        }
        if (r == 0) {
            if (!in->func_08054138()) {
                func_0804acf4("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = quiet_080bbd98;
                in->leDump();
                errorOccurred = saved;
            }
            if (!t->destroyInstruction(in)) {
                func_0804ad38("failed to destroy instruction\n");
                if (in)
                    in->func_08054008(false);
                return false;
            }
        }
    }
    return true;
}

#endif
