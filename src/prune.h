// prune pass helpers; inline, emitted out-of-line via #pragma implementation in prune.cpp.
#ifndef PRUNE_H
#define PRUNE_H

#pragma interface

#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "VUStr.h"

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
            int isIn = op->unk_09 == 1;
            RegPoolEntry *r = isIn ? op->unk_18 : 0;
            if (r)
                op->unk_2a = op->unk_07;
        }
    }
    Operand *out;
    for (int o = 0; (out = in->getOp(o)) != 0; o++) {
        int isOut = out->unk_09 == 2;
        RegPoolEntry *reg = isOut ? out->unk_18 : 0;
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
                int sw = ol->unk_11;
                if (src->unk_18 != reg) {
                    func_0804ad38("there was a reg mismatch\n");
                    return -1;
                }
                if (sw != s) {
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
            int isIn = ip->unk_09 == 1;
            RegPoolEntry *r = isIn ? ip->unk_18 : 0;
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

int func_08087860(void *, Instr *in);

inline int doPrune(Instr *in)
{
    unsigned char any = 0;
    if (in->unk_13cc)
        return 1;
    for (int i = 0;; i++) {
        Operand *op = in->getOp(i);
        if (!op)
            break;
        any |= op->unk_2a;
    }
    if (!any)
        return 0;
    iInfo *info = in->unk_34;
    if (!info) {
        func_0804ad38("Instruction::doPrune() - had no iInfo\n");
        return -1;
    }
    if (in->unk_142c && in->unk_38[0].unk_18 == in->unk_38[1].unk_18)
        return 0;
    if (info->unk_0c == 1) {
        int m, nIn = 0, inOR = 0, outOR = 0, nOut = 0;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            if (op->unk_04 != 1 && op->unk_04 != 9)
                continue;
            if (!op->unk_18) {
                func_0804ad38("couldn't get a register for V_FL4 or V_ACC operand\n");
                return -1;
            }
            if (op->unk_09 == 1) {
                inOR |= op->unk_2a;
                nIn++;
            }
            if (op->unk_09 == 2) {
                if (nOut && op->unk_2a != outOR) {
                    func_0804ad38("prune - found two inputs with different pBits state for FL4/ACC\n");
                    return -1;
                }
                outOR |= op->unk_2a;
                nOut++;
            }
        }
        if (!nOut && !nIn) {
            func_0804ad38("MD_ANY set - but couldn't find any fl4 or acc operands for %s\n", info->unk_04);
            return -1;
        }
        if (nOut > 1) {
            func_0804ad38("nOut >2! for fl4/acc\n ");
            in->leDump();
            return -1;
        }
        if (info->unk_20 == func_08087860)
            inOR = (inOR >> 1) | ((inOR & 1) << 3);
        if (nIn) {
            if (outOR & ~inOR) {
                func_0804ad38("doPrune:: output had larger extent than input.. out=%x vs in=%x\n", outOR, inOR);
                in->leDump();
                return -1;
            }
            m = inOR;
        } else if (!outOR)
            m = in->unk_138f;
        else
            m = outOR;
        if (m & ~in->unk_138f) {
            func_0804ad38("inOR was out of range of instruction mod bits nb=%x,mb=%x!\n", m, in->unk_138f);
            in->func_08054008(false);
            return -1;
        }
        if (m != in->unk_138f) {
            func_0804acf4("PRUNE FIELD: [%s->%s]  ", func_08052064(in->unk_138f), func_08052064(m));
            int saved = errorOccurred;
            errorOccurred = quiet_080bbd98;
            in->leDump();
            errorOccurred = saved;
        }
        in->unk_138f = m;
        for (int i = 0;; i++) {
            Operand *op = in->getOp(i);
            if (!op)
                break;
            if (op->unk_04 == 1 || op->unk_04 == 9)
                op->unk_07 = m;
        }
        if (info->unk_20 == func_08087860)
            in->unk_38[1].unk_07 = (((m & 8) >> 3) | (m << 1)) & 0xf;
    }
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *r = op->isOutRegOp() ? op->unk_18 : 0;
        if (!r || op->unk_2a || !r->unk_14)
            continue;
        RegPool *pool = r->unk_04;
        if (!pool) {
            func_0804ad38("register had no regpool\n");
            return -1;
        }
        if (pool->unk_10 && !op->unk_0d) {
            func_0804acf4("ZERO FIX   : ");
            int saved = errorOccurred;
            errorOccurred = quiet_080bbd98;
            in->leDump(i);
            errorOccurred = saved;
            op->unk_18 = pool->unk_10;
        }
    }
    return 1;
}

#endif
