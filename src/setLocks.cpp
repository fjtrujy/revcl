#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

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
