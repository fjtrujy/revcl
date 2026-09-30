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
