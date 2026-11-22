#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"

bool unlinkFromPendingInstructions(Block *blk, Instr *n);
bool appendCompletedInstruction(Block *blk, Instr *n);

bool ScheduleTable::forceInsert(Instr *candidateInstruction, InstructionPosition *position)
{
    int operandIndex = 0;
    if (!candidateInstruction) {
        reportError("forceInsert - null in\n");
        return false;
    }
    InstructionPosition *instructionPosition = &candidateInstruction->position;
    int row = position->loopRow;
    int pipe = position->pipelineIndex;
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
    Instr **instructionSlot = ((InstructionOwnerTableView *)this)->getIE_ck(row);
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
    instructionPosition->linearRow = position->linearRow;
    instructionPosition->loopRow = position->loopRow;
    instructionPosition->loopIteration = position->loopIteration;
    instructionPosition->pipelineIndex = position->pipelineIndex;
    *slot = candidateInstruction;
    candidateInstruction->insertionSerial = nextInsertionSerial;
    nextInsertionSerial++;
    for (operandIndex = 0; operandIndex != candidateInstruction->operandCount; operandIndex++) {
        Operand *operand = &candidateInstruction->operands[operandIndex];
        RegisterEntry *registerEntry = registerOperand(operand, 1);
        if (!registerEntry)
            continue;
        for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
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
                operandLock->setFrom(position);
                if (!registerPartLocks->addInputLock(operandLock)) {
                    reportError("failed to add input lock\n");
                    return false;
                }
            }
        }
    }
    for (operandIndex = 0; operandIndex != candidateInstruction->operandCount; operandIndex++) {
        Operand *outputOperand = &candidateInstruction->operands[operandIndex];
        RegisterEntry *registerEntry = registerOperand(outputOperand, 2);
        if (!registerEntry)
            continue;
        for (int outputPartIndex = 0; outputPartIndex != registerEntry->partCount; outputPartIndex++) {
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
                outputLock->setFrom(position);
                if (!outputLockList->addOutputLock(outputLock)) {
                    reportError("failed to add output lock\n");
                    return false;
                }
            }
        }
    }
    if (!unlinkFromPendingInstructions((Block *)this, candidateInstruction)) {
        reportError("failed to unlink from the todo list\n");
        return false;
    }
    if (!appendCompletedInstruction((Block *)this, candidateInstruction)) {
        reportError("failed to add instruction to the done list tail\n");
        return false;
    }
    return true;
}

bool unlinkFromCompletedInstructions(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);
