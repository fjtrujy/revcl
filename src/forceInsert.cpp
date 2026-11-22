#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"

bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);

bool workTable::forceInsert(Instr *in, InstructionPosition *pos)
{
    int operandIndex = 0;
    if (!in) {
        reportError("forceInsert - null in\n");
        return false;
    }
    InstructionPosition *instructionPosition = &in->position;
    int row = pos->loopRow;
    int pipe = pos->pipelineIndex;
    if (instructionPosition->loopRow >= 0) {
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
    Instr **instructionSlot = ((cls_unk30 *)this)->getIE_ck(row);
    if (!instructionSlot) {
        reportError("couldn't get ie\n");
        return false;
    }
    if (pipe < 0 || pipe > 1) {
        reportError("out of bounds pipe\n");
        return false;
    }
    Instr **slot = &instructionSlot[pipe];
    if (*slot != (Instr *)in->replacedInstructionWord) {
        reportError("instruction already inserted or bad key\n");
        return false;
    }
    instructionPosition->linearRow = pos->linearRow;
    instructionPosition->loopRow = pos->loopRow;
    instructionPosition->loopIteration = pos->loopIteration;
    instructionPosition->pipelineIndex = pos->pipelineIndex;
    *slot = in;
    in->insertionSerial = nextInsertionSerial;
    nextInsertionSerial++;
    for (operandIndex = 0; operandIndex != in->operandCount; operandIndex++) {
        Operand *operand = &in->operands[operandIndex];
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (!reg)
            continue;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((bool)((operand->componentMask >> partIndex) & 1)) {
                OperandLock *operandLock = &operand->parts[partIndex];
                if (!operandLock) {
                    reportError("failed to get lock \n");
                    return false;
                }
                RegisterPartLockList *registerPartLocks = operandLock->ownerLockList;
                if (!registerPartLocks) {
                    reportError("lock had no lock list\n");
                    return false;
                }
                operandLock->setFrom(pos);
                if (!registerPartLocks->addInputLock(operandLock)) {
                    reportError("failed to add input lock\n");
                    return false;
                }
            }
        }
    }
    for (operandIndex = 0; operandIndex != in->operandCount; operandIndex++) {
        Operand *op = &in->operands[operandIndex];
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
