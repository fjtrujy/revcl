#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"

bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);

bool workTable::forceInsert(Instr *in, cls08058718 *pos)
{
    int i = 0;
    if (!in) {
        func_0804ad38("forceInsert - null in\n");
        return false;
    }
    cls08058718 *p = &in->unk_13e8;
    int row = pos->unk_04;
    int pipe = pos->unk_02;
    if (p->unk_04 >= 0) {
        func_0804ad38("instruction already inserted\n");
        return false;
    }
    if (row < unk_30) {
        func_0804ad38("ForceInsert - before start\n");
        return false;
    }
    if (row >= unk_34) {
        func_0804ad38("ForceInsert - after end\n");
        return false;
    }
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
    if (!ie) {
        func_0804ad38("couldn't get ie\n");
        return false;
    }
    if (pipe < 0 || pipe > 1) {
        func_0804ad38("out of bounds pipe\n");
        return false;
    }
    Instr **slot = &ie[pipe];
    if (*slot != (Instr *)in->unk_1398) {
        func_0804ad38("instruction already inserted or bad key\n");
        return false;
    }
    p->unk_00 = pos->unk_00;
    p->unk_04 = pos->unk_04;
    p->unk_03 = pos->unk_03;
    p->unk_02 = pos->unk_02;
    *slot = in;
    in->unk_1396 = unk_40;
    unk_40++;
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((bool)((op->unk_07 >> k) & 1)) {
                cls0805bcc4 *lk = &op->unk_2c[k];
                if (!lk) {
                    func_0804ad38("failed to get lock \n");
                    return false;
                }
                cls0805c244 *ll = lk->unk_04;
                if (!ll) {
                    func_0804ad38("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addInputLock(lk)) {
                    func_0804ad38("failed to add input lock\n");
                    return false;
                }
            }
        }
    }
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *reg = registerOperand(op, 2);
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((bool)((op->unk_07 >> k) & 1)) {
                cls0805bcc4 *lk = &op->unk_2c[k];
                if (!lk) {
                    func_0804ad38("failed to get lock \n");
                    return false;
                }
                cls0805c244 *ll = lk->unk_04;
                if (!ll) {
                    func_0804ad38("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addOutputLock(lk)) {
                    func_0804ad38("failed to add output lock\n");
                    return false;
                }
            }
        }
    }
    if (!UnlinkFromToDo((Block *)this, in)) {
        func_0804ad38("failed to unlink from the todo list\n");
        return false;
    }
    if (!AddDone_Tail((Block *)this, in)) {
        func_0804ad38("failed to add instruction to the done list tail\n");
        return false;
    }
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);
