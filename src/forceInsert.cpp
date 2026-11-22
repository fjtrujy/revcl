#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"

bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);

bool workTable::forceInsert(Instr *candidateInstruction, InstructionPosition *pos)
{
    int operandIndex = 0;
    if (!candidateInstruction) {
        reportError("forceInsert - null in\n");
        return false;
    }
    InstructionPosition *instructionPosition = &candidateInstruction->position;
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
    if (*slot != (Instr *)candidateInstruction->replacedInstructionWord) {
        reportError("instruction already inserted or bad key\n");
        return false;
    }
    instructionPosition->linearRow = pos->linearRow;
    instructionPosition->loopRow = pos->loopRow;
    instructionPosition->loopIteration = pos->loopIteration;
    instructionPosition->pipelineIndex = pos->pipelineIndex;
    *slot = candidateInstruction;
    candidateInstruction->insertionSerial = nextInsertionSerial;
    nextInsertionSerial++;
    for (operandIndex = 0; operandIndex != candidateInstruction->operandCount; operandIndex++) {
        Operand *operand = &candidateInstruction->operands[operandIndex];
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
    for (operandIndex = 0; operandIndex != candidateInstruction->operandCount; operandIndex++) {
        Operand *outputOperand = &candidateInstruction->operands[operandIndex];
        RegPoolEntry *reg = registerOperand(outputOperand, 2);
        if (!reg)
            continue;
        for (int outputPartIndex = 0; outputPartIndex != reg->partCount; outputPartIndex++) {
            if ((bool)((outputOperand->componentMask >> outputPartIndex) & 1)) {
                OperandLock *outputLock = &outputOperand->parts[outputPartIndex];
                if (!outputLock) {
                    reportError("failed to get lock \n");
                    return false;
                }
                RegisterPartLockList *outputLockList = outputLock->ownerLockList;
                if (!outputLockList) {
                    reportError("lock had no lock list\n");
                    return false;
                }
                outputLock->setFrom(pos);
                if (!outputLockList->addOutputLock(outputLock)) {
                    reportError("failed to add output lock\n");
                    return false;
                }
            }
        }
    }
    if (!UnlinkFromToDo((Block *)this, candidateInstruction)) {
        reportError("failed to unlink from the todo list\n");
        return false;
    }
    if (!AddDone_Tail((Block *)this, candidateInstruction)) {
        reportError("failed to add instruction to the done list tail\n");
        return false;
    }
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);
