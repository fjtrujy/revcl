#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

class Block;
bool func_0806558c(Instr *i, int *row);
bool unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);
bool UnlinkFromDone(Block *blk, Instr *n);
bool AddToDo_Head(Block *blk, Instr *n);
bool setRange(int start, int end, int lsz);
int func_0807e130(workTable *wt, Instr *in, cls08058718 *pos, bool upwards);
int func_0804bf50(unsigned int x);
extern bool stagger_080b33ac;

bool setLocks(Instr *in)
{
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->unk_18;
        if (!r)
            continue;
        if (op->isOutRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    cls0805c244 *ll = s->unk_04;
                    s->setFrom(&in->unk_13e8);
                    if (!ll->addOutputLock(s)) {
                        func_0804ad38("oh no - add output lock failed %d.%d\n", i, p);
                        in->func_08054008(false);
                        return false;
                    }
                }
            }
        }
        if (op->isInRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    cls0805c244 *ll = s->unk_04;
                    s->setFrom(&in->unk_13e8);
                    if (!ll->addInputLock(s)) {
                        func_0804ad38("SetLocks: oh no - add input lock failed %d.%d\n", i, p);
                        in->func_08054008(false);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool undoLocks(Instr *in)
{
    for (int i = in->unk_138b - 1; i >= 0; i--) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->unk_18;
        if (!r)
            continue;
        if (op->isOutRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                op->unk_2c[p].unk_52[0] = 0;
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    if (!s->unk_04->undoOutputLock(s))
                        return false;
                }
            }
        }
        if (op->isInRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    if (!s->unk_04->undoInputLock(s))
                        return false;
                }
            }
        }
    }
    return true;
}

int checkLocks(Instr *in, short *pos, int *minPos)
{
    cls0805bcc4 *saved[64];
    int ret = 0;
    int i;
    int n = 0;

    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->isInRegOp() ? op->unk_18 : 0;
        if (!r)
            continue;
        for (int p = 0; p != r->unk_28; p++) {
            if ((op->unk_07 >> p) & 1) {
                OpSlot *s = &op->unk_2c[p];
                cls0805c244 *ll = s->unk_04;
                if (*minPos < ll->unk_00) {
                    ret |= 0x80000;
                    *minPos = ll->unk_00;
                    goto done;
                }
                int rc = ll->checkInputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x430) {
                    cls0805bcc4 *par = s->getParent();
                    Operand *o = par->ownerOperand();
                    if (o) {
                        Instr *oi = par->ownerInstr();
                        if (!o->unk_2c[p].unk_52[0]) {
                            o->unk_2c[p].unk_52[0] = 1;
                            o->unk_2c[p].unk_50++;
                        }
                        func_080737b8(oi);
                        oi->func_08054008(false);
                    } else
                        func_0804ac7c("failed to get setting operand .. so it's a regref??\n");
                    *minPos = 30000;
                    goto done;
                }
                if (s->unk_1c) {
                    cls0805bcc4 *par = s->getParent();
                    if (!par)
                        func_0804adf0(0);
                    par->decRef();
                    saved[n++] = par;
                }
            }
        }
    }
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->isOutRegOp() ? op->unk_18 : 0;
        if (!r)
            continue;
        for (int p = 0; p != r->unk_28; p++) {
            if ((op->unk_07 >> p) & 1) {
                OpSlot *s = &op->unk_2c[p];
                cls0805c244 *ll = op->unk_2c[p].unk_04;
                if (op->unk_2c[p].unk_50) {
                    int v = ll->func_0805c938(op->unk_2c[p].unk_50);
                    if (v > *minPos) {
                        ret |= 0x80000;
                        *minPos = v;
                        goto done;
                    }
                }
                int rc = ll->checkOutputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x22000) {
                    if (!g_080b336c) {
                        func_0804ad38("late extern, loop overrun, but no loop\n");
                        func_0804aeb0();
                    }
                    int d = *pos + s->unk_18 - g_080b336c;
                    if (d > 0) {
                        ll->unk_00 = d;
                        Instr *e = ll->earliestExt();
                        if (!e) {
                            func_0804ac7c("failed to get earliest in ret=%x\n", rc);
                            ll->checkCounts();
                            func_0804ad38("this instruction is\n");
                            in->leDump(i, p);
                            func_0804aeb0();
                        } else
                            func_080737b8(e);
                        *minPos = 30000;
                        goto done;
                    }
                }
            }
        }
    }
done:
    for (int k = 0; k != n; k++)
        saved[k]->incRef();
    return ret;
}

int canMoveLocks(Instr *in, short *pos)
{
    cls0805bcc4 *saved[64];
    int ret = 0;
    int i;
    int n = 0;

    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->inRegOp();
        if (!r)
            continue;
        for (int p = 0; p != r->unk_28; p++) {
            if ((op->unk_07 >> p) & 1) {
                cls0805bcc4 *par = op->unk_2c[p].getParent();
                if (!par->calcStats(in)) {
                    func_0804ad38("failed to recalc stats ignoring instruction\n");
                    func_0804aeb0();
                }
                saved[n++] = par;
            }
        }
    }
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *ri = op->inRegOp();
        if (ri) {
            for (int p = 0; p != ri->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    int *minPos;
                    ret |= s->unk_04->canMoveInputLock(s, pos, minPos);
                    if (ret)
                        goto done;
                }
            }
        }
        RegPoolEntry *ro = op->outRegOp();
        if (ro) {
            for (int p = 0; p != ro->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    int *minPos;
                    ret |= s->unk_04->canMoveOutputLock(s, pos, minPos);
                    if (ret)
                        goto done;
                }
            }
        }
    }
done:
    while (n != 0) {
        if (!saved[--n]->calcStats(0)) {
            func_0804ad38("failed to calc stats undoing move test\n");
            func_0804aeb0();
        }
    }
    return ret;
}

bool moveLocks(Instr *in, cls08058718 *pos)
{
    cls08058718 *d = &in->unk_13e8;
    d->unk_00 = pos->unk_00;
    d->unk_04 = pos->unk_04;
    d->unk_03 = pos->unk_03;
    d->unk_02 = pos->unk_02;
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->unk_18;
        if (!r)
            continue;
        if (op->isOutRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    cls0805c244 *ll = s->unk_04;
                    s->setFrom(&in->unk_13e8);
                    if (!ll->moveOutputLock(s)) {
                        func_0804ad38("oh no - move output lock failed %d.%d\n", i, p);
                        in->func_08054008(false);
                        func_0804aeb0();
                        return false;
                    }
                }
            }
        }
        if (op->isInRegOp()) {
            for (int p = 0; p != r->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *s = &op->unk_2c[p];
                    cls0805c244 *ll = s->unk_04;
                    s->setFrom(&in->unk_13e8);
                    if (!ll->moveInputLock(s)) {
                        func_0804ad38("oh no - move input lock failed %d.%d\n", i, p);
                        in->func_08054008(false);
                        func_0804aeb0();
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool setPipeBits(Instr *in)
{
    int i;
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_18)
            op->unk_18->unk_34 = 0;
    }
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *r = op->inputReg();
        if (!r)
            continue;
        for (int p = 0; p != r->unk_28; p++) {
            if ((op->unk_07 >> p) & 1) {
                OpSlot *s = &op->unk_2c[p];
                s->unk_01 = 0;
                s->unk_00 = 0;
                s->unk_02 = 0;
                if (!(((unsigned char)r->unk_34 >> p) & 1)) {
                    switch ((unsigned char)in->unk_138c) {
                    case 0:
                        func_0804ad38("no physical pipe bits!\n");
                        return false;
                    case 1:
                        s->unk_01 = 1;
                        break;
                    case 2:
                        s->unk_00 = 1;
                        break;
                    case 3:
                        s->unk_02 = 1;
                        break;
                    default:
                        func_0804ad38("unknown pipe request\n");
                        return false;
                    }
                }
            }
        }
        r->unk_34 |= op->unk_07;
    }
    return true;
}

int checkInsert(workTable *wt, Instr *in, cls08058718 *pos, int *minPos)
{
    int row = pos->unk_00;
    int pipe = pos->unk_02;
    int r4 = pos->unk_04;

    if (*minPos < row)
        *minPos = row;

    short e = in->unk_13b2;
    if (e < in->unk_13b0)
        e = in->unk_13b0;
    int v = e;
    if (r4 < v + g_080b3370) {
        int m = *minPos;
        if (g_080b336c) {
            int rem = v % g_080b336c;
            int q = m / g_080b336c;
            if (rem <= m % g_080b336c)
                q++;
            v = q * g_080b336c + rem;
        }
        *minPos = v;
        return 0x10000;
    }

    short l = in->unk_13b6;
    if (l < in->unk_13b4)
        l = in->unk_13b4;
    if (l && g_080b3374 >= 0 && g_080b3374 - r4 <= l) {
        *minPos = 30000;
        return 0x10000;
    }
    if (stagger_080b33ac && in->unk_13ce && (bool)(r4 & 1)) {
        *minPos = row + 1;
        return 0x10000;
    }

    if (wt->unk_38) {
        if (row / wt->unk_38 > 16) {
            *minPos = 30000;
            return 0x8000;
        }
    } else if (row >= wt->unk_28) {
        *minPos = 30000;
        return 0x8000;
    }

    int fr = 0;
    if (!in->unk_1410) {
        if (!func_0806558c(in, &fr)) {
            func_0804ad38("check fdeps failed\n");
            func_0804adf0(-1);
            return 0x80000000;
        }
        if (fr < 0) {
            *minPos = 30000;
            return 0x100000;
        }
        if (fr > *minPos) {
            *minPos = fr;
            return 0x100000;
        }
    }

    Instr **ie = ((cls_unk30 *)wt)->getIE_ck(pos->unk_04);
    if (ie[pipe] != (Instr *)in->unk_1398) {
        *minPos = row + 1;
        return 0x40;
    }
    if (!wouldPair(ie, in, pipe)) {
        (*minPos)++;
        return 0x80;
    }
    return checkLocks(in, &pos->unk_00, minPos);
}

bool DoInsert(workTable *wt, Instr *in, cls08058718 *pos)
{
    if (in->unk_142c && !in->unk_142e) {
        func_0804ad38("undo insert currmuid not set\n");
        return false;
    }
    wtRow *row = &wt->unk_2c[pos->unk_04];
    cls08058718 *s = &in->unk_13e8;
    s->unk_04 = pos->unk_04;
    s->unk_03 = pos->unk_03;
    s->unk_00 = pos->unk_00;
    s->unk_02 = pos->unk_02;
    row->unk_00[pos->unk_02].unk_00 = in;
    if (!setLocks(in)) {
        func_0804ad38("DoInsert: failed to set locks in instruction insert\n");
        return false;
    }
    in->unk_1396 = wt->unk_40++;
    if (!UnlinkFromToDo((Block *)wt, in)) {
        func_0804ad38("failed to unlink from to do list\n");
        func_0804aeb0();
    }
    if (!AddDone_Tail((Block *)wt, in)) {
        func_0804ad38("failed to add to done\n");
        func_0804aeb0();
    }
    return true;
}

bool UndoInsert(workTable *wt, Instr *in)
{
    if (in->unk_1388)
        wt->unk_84 = 0;
    if (in->unk_142c) {
        if (!in->unk_142e) {
            func_0804ad38("undo insert currmuid not set\n");
            return false;
        }
        if (!unresolveMove((Block *)wt, in)) {
            func_0804ad38("failed to unresolve move\n");
            func_0804aeb0();
        }
    }
    if (!wt->takeOut(in)) {
        func_0804ad38("failed to take out instruction!\n");
        func_0804aeb0();
    }
    if (!undoLocks(in)) {
        func_0804ad38("UndoInsert:: undo locks - failed\n");
        func_0804aeb0();
        return false;
    }
    wt->unk_40--;
    in->unk_1396 = -1;
    if (!UnlinkFromDone((Block *)wt, in)) {
        func_0804ad38("failed to unlink from done list\n");
        func_0804aeb0();
    }
    if (!AddToDo_Head((Block *)wt, in)) {
        func_0804ad38("failed to relink to todo list\n");
        func_0804aeb0();
    }
    return true;
}

bool reduceExtents(workTable *wt)
{
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);
    int moved;
    do {
        moved = 0;
        for (int r = wt->unk_30; r != wt->unk_34; r++) {
            wtRow *row = wt->func_080724f0(r);
            for (int p = 0; p != 2; p++) {
                Instr *in = row->getEntry(p);
                if (!in)
                    continue;
                int lo = in->unk_13e8.unk_00;
                int cnt = 0;
                int minEnd = 30000;
                Operand *op;
                for (int i = 0; (op = in->getOp(i)) != 0; i++) {
                    RegPoolEntry *reg = op->outputRegZ();
                    if (reg) {
                        for (int k = 0; k != reg->unk_28; k++) {
                            if ((op->unk_07 >> k) & 1) {
                                cnt++;
                                OpSlot *s = &op->unk_2c[k];
                                if (s->unk_20 != 0) {
                                    int e = s->unk_3e;
                                    if (e < minEnd)
                                        minEnd = e;
                                }
                            }
                        }
                    }
                    if (op->inputReg())
                        cnt -= func_0804bf50(op->unk_07);
                }
                if (minEnd < 30000 && cnt >= 0) {
                    int t = minEnd;
                    cls08058718 pos;
                    bool found = false;
                    for (; t > lo; t--) {
                        pos.func_0805874c(t);
                        if (!func_0807e130(wt, in, &pos, false)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        if (!wt->takeOut(in)) {
                            func_0804ad38("failed to take out instruction\n");
                            func_0804aeb0();
                        }
                        if (!moveLocks(in, &pos)) {
                            func_0804ad38("failed to move !\n");
                            func_0804aeb0();
                        }
                        if (!wt->putIn(in)) {
                            func_0804ad38("failed to put in back in\n");
                            func_0804aeb0();
                        }
                        moved++;
                    }
                }
            }
        }
    } while (moved);
    return true;
}
