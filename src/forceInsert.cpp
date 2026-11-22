#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"

bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);

bool workTable::forceInsert(Instr *in, InstructionPosition *pos)
{
    int i = 0;
    if (!in) {
        reportError("forceInsert - null in\n");
        return false;
    }
    InstructionPosition *p = &in->position;
    int row = pos->unk_04;
    int pipe = pos->unk_02;
    if (p->unk_04 >= 0) {
        reportError("instruction already inserted\n");
        return false;
    }
    if (row < rangeStart) {
        reportError("ForceInsert - before start\n");
        return false;
    }
    if (row >= rangeEnd) {
        reportError("ForceInsert - after end\n");
        return false;
    }
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
    if (!ie) {
        reportError("couldn't get ie\n");
        return false;
    }
    if (pipe < 0 || pipe > 1) {
        reportError("out of bounds pipe\n");
        return false;
    }
    Instr **slot = &ie[pipe];
    if (*slot != (Instr *)in->unk_1398) {
        reportError("instruction already inserted or bad key\n");
        return false;
    }
    p->unk_00 = pos->unk_00;
    p->unk_04 = pos->unk_04;
    p->unk_03 = pos->unk_03;
    p->unk_02 = pos->unk_02;
    *slot = in;
    in->insertionSerial = unk_40;
    unk_40++;
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((bool)((op->componentMask >> k) & 1)) {
                OperandLock *lk = &op->parts[k];
                if (!lk) {
                    reportError("failed to get lock \n");
                    return false;
                }
                RegisterPartLockList *ll = lk->ownerLockList;
                if (!ll) {
                    reportError("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addInputLock(lk)) {
                    reportError("failed to add input lock\n");
                    return false;
                }
            }
        }
    }
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *reg = registerOperand(op, 2);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((bool)((op->componentMask >> k) & 1)) {
                OperandLock *lk = &op->parts[k];
                if (!lk) {
                    reportError("failed to get lock \n");
                    return false;
                }
                RegisterPartLockList *ll = lk->ownerLockList;
                if (!ll) {
                    reportError("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addOutputLock(lk)) {
                    reportError("failed to add output lock\n");
                    return false;
                }
            }
        }
    }
    if (!UnlinkFromToDo((Block *)this, in)) {
        reportError("failed to unlink from the todo list\n");
        return false;
    }
    if (!AddDone_Tail((Block *)this, in)) {
        reportError("failed to add instruction to the done list tail\n");
        return false;
    }
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);
