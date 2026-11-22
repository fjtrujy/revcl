#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

class Block;
bool calculateEarliestDependencyRow(Instr *i, int *row);
bool calculateLatestDependencyRow(Instr *i, int *row);
bool unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);
bool UnlinkFromDone(Block *blk, Instr *n);
bool AddToDo_Head(Block *blk, Instr *n);
bool setRange(int start, int end, int lsz);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int pullSpecialRegisterWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int pullLatencyWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int countSetBits(unsigned int x);
extern bool staggerMemoryAccess;

static inline void dumpLockInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

bool setLocks(Instr *instruction)
{
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = operand->assignedRegister;
        if (!registerEntry)
            continue;
        if (isRegisterOperand(operand, 2)) {
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                if ((operand->componentMask >> partIndex) & 1) {
                    OpSlot *slot = &operand->parts[partIndex];
                    RegisterPartLockList *lockList = slot->ownerLockList;
                    slot->setFrom(&instruction->position);
                    if (!lockList->addOutputLock(slot)) {
                        reportError("oh no - add output lock failed %d.%d\n", operandIndex, partIndex);
                        dumpLockInstruction(instruction);
                        return false;
                    }
                }
            }
        }
        if (isRegisterOperand(operand, 1)) {
            for (int p = 0; p != registerEntry->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    RegisterPartLockList *lockList = slot->ownerLockList;
                    slot->setFrom(&instruction->position);
                    if (!lockList->addInputLock(slot)) {
                        reportError("SetLocks: oh no - add input lock failed %d.%d\n", operandIndex, p);
                        dumpLockInstruction(instruction);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool undoLocks(Instr *instruction)
{
    for (int operandIndex = instruction->operandCount - 1; operandIndex >= 0; operandIndex--) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = operand->assignedRegister;
        if (!registerEntry)
            continue;
        if (isRegisterOperand(operand, 2)) {
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                operand->parts[partIndex].outputConflictMarked[0] = 0;
                if ((operand->componentMask >> partIndex) & 1) {
                    OpSlot *slot = &operand->parts[partIndex];
                    if (!slot->ownerLockList->undoOutputLock(slot))
                        return false;
                }
            }
        }
        if (isRegisterOperand(operand, 1)) {
            for (int p = 0; p != registerEntry->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    if (!slot->ownerLockList->undoInputLock(slot))
                        return false;
                }
            }
        }
    }
    return true;
}

int checkLocks(Instr *instruction, short *pos, int *minPos)
{
    OperandLock *saved[64];
    int ret = 0;
    int operandIndex;
    int conflictingLockCount = 0;

    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = isRegisterOperand(operand, 1) ? operand->assignedRegister : 0;
        if (!registerEntry)
            continue;
        for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
            if ((operand->componentMask >> partIndex) & 1) {
                OpSlot *slot = &operand->parts[partIndex];
                RegisterPartLockList *lockList = slot->ownerLockList;
                if (*minPos < lockList->loopWrapMinimumRow) {
                    ret |= 0x80000;
                    *minPos = lockList->loopWrapMinimumRow;
                    goto done;
                }
                int checkResult = lockList->checkInputLock(slot, pos, minPos);
                ret |= checkResult;
                if (checkResult & 0x430) {
                    OperandLock *par = slot->getParent();
                    Operand *otherOperand = par->ownerOperand();
                    if (otherOperand) {
                        Instr *otherInstruction = par->ownerInstr();
                        if (!otherOperand->parts[partIndex].outputConflictMarked[0]) {
                            otherOperand->parts[partIndex].outputConflictMarked[0] = 1;
                            otherOperand->parts[partIndex].conflictingOutputLockCount++;
                        }
                        trackEarliestInsertion(otherInstruction);
                        otherInstruction->dumpOperandDiagnostics(false);
                    } else
                        reportWarning("failed to get setting operand .. so it's a regref??\n");
                    *minPos = 30000;
                    goto done;
                }
                if (slot->pairedLock) {
                    OperandLock *par = slot->getParent();
                    if (!par)
                        exitAfterCleanup(0);
                    par->decRef();
                    saved[conflictingLockCount++] = par;
                }
            }
        }
    }
    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = isRegisterOperand(operand, 2) ? operand->assignedRegister : 0;
        if (!registerEntry)
            continue;
        for (int p = 0; p != registerEntry->partCount; p++) {
            if ((operand->componentMask >> p) & 1) {
                OpSlot *slot = &operand->parts[p];
                RegisterPartLockList *lockList = operand->parts[p].ownerLockList;
                if (operand->parts[p].conflictingOutputLockCount) {
                    int earliestAllowedRow = lockList->maxConstraintOfFirstOutputs(operand->parts[p].conflictingOutputLockCount);
                    if (earliestAllowedRow > *minPos) {
                        ret |= 0x80000;
                        *minPos = earliestAllowedRow;
                        goto done;
                    }
                }
                int rc = lockList->checkOutputLock(slot, pos, minPos);
                ret |= rc;
                if (rc & 0x22000) {
                    if (!activeLoopSize) {
                        reportError("late extern, loop overrun, but no loop\n");
                        abortAfterError();
                    }
                    int loopWrapDistance = *pos + slot->issueLatency - activeLoopSize;
                    if (loopWrapDistance > 0) {
                        lockList->loopWrapMinimumRow = loopWrapDistance;
                        Instr *endInstruction = lockList->earliestExt();
                        if (!endInstruction) {
                            reportWarning("failed to get earliest in ret=%x\n", rc);
                            lockList->checkCounts();
                            reportError("this instruction is\n");
                            instruction->leDump(operandIndex, p);
                            abortAfterError();
                        } else
                            trackEarliestInsertion(endInstruction);
                        *minPos = 30000;
                        goto done;
                    }
                }
            }
        }
    }
done:
    for (int k = 0; k != conflictingLockCount; k++)
        saved[k]->incRef();
    return ret;
}

int canMoveLocks(Instr *instruction, short *pos)
{
    OperandLock *saved[64];
    int ret = 0;
    int operandIndex;
    int conflictingLockCount = 0;

    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = registerOperand(operand, 1);
        if (!registerEntry)
            continue;
        for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
            if ((operand->componentMask >> partIndex) & 1) {
                OperandLock *par = operand->parts[partIndex].getParent();
                if (!par->calcStats(instruction)) {
                    reportError("failed to recalc stats ignoring instruction\n");
                    abortAfterError();
                }
                saved[conflictingLockCount++] = par;
            }
        }
    }
    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = registerOperand(operand, 1);
        if (registerEntry) {
            for (int p = 0; p != registerEntry->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    ret |= slot->ownerLockList->canMoveInputLock(slot, pos, (operand->componentMask >> p) & 1);
                    if (ret)
                        goto done;
                }
            }
        }
        registerEntry = registerOperand(operand, 2);
        if (registerEntry) {
            for (int p = 0; p != registerEntry->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    ret |= slot->ownerLockList->canMoveOutputLock(slot, pos, (operand->componentMask >> p) & 1);
                    if (ret)
                        goto done;
                }
            }
        }
    }
done:
    while (conflictingLockCount != 0) {
        if (!saved[--conflictingLockCount]->calcStats(0)) {
            reportError("failed to calc stats undoing move test\n");
            abortAfterError();
        }
    }
    return ret;
}

bool moveLocks(Instr *const instruction, InstructionPosition *pos)
{
    InstructionPosition *position = &instruction->position;
    position->linearRow = pos->linearRow;
    position->loopRow = pos->loopRow;
    position->loopIteration = pos->loopIteration;
    position->pipelineIndex = pos->pipelineIndex;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = operand->assignedRegister;
        if (!registerEntry)
            continue;
        if (isRegisterOperand(operand, 2)) {
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                if ((operand->componentMask >> partIndex) & 1) {
                    OpSlot *slot = &operand->parts[partIndex];
                    RegisterPartLockList *lockList = slot->ownerLockList;
                    slot->setFrom(&instruction->position);
                    if (!lockList->moveOutputLock(slot)) {
                        reportError("oh no - move output lock failed %d.%d\n", operandIndex, partIndex);
                        dumpLockInstruction(instruction);
                        abortAfterError();
                        return false;
                    }
                }
            }
        }
        if (isRegisterOperand(operand, 1)) {
            for (int p = 0; p != registerEntry->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    RegisterPartLockList *lockList = slot->ownerLockList;
                    slot->setFrom(&instruction->position);
                    if (!lockList->moveInputLock(slot)) {
                        reportError("oh no - move input lock failed %d.%d\n", operandIndex, p);
                        dumpLockInstruction(instruction);
                        abortAfterError();
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool setPipeBits(Instr *instruction)
{
    int operandIndex;
    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        if (operand->assignedRegister)
            operand->assignedRegister->seenInputComponentMask = 0;
    }
    for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = operand->inputRegister();
        if (!registerEntry)
            continue;
        for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
            if ((operand->componentMask >> partIndex) & 1) {
                OpSlot *slot = &operand->parts[partIndex];
                slot->upperPipeInputDemand = 0;
                slot->lowerPipeInputDemand = 0;
                slot->eitherPipeInputDemand = 0;
                if (!(((unsigned char)registerEntry->seenInputComponentMask >> partIndex) & 1)) {
                    switch ((unsigned char)instruction->permittedPipelineMask) {
                    case 0:
                        reportError("no physical pipe bits!\n");
                        return false;
                    case 1:
                        slot->upperPipeInputDemand = 1;
                        break;
                    case 2:
                        slot->lowerPipeInputDemand = 1;
                        break;
                    case 3:
                        slot->eitherPipeInputDemand = 1;
                        break;
                    default:
                        reportError("unknown pipe request\n");
                        return false;
                    }
                }
            }
        }
        registerEntry->seenInputComponentMask |= operand->componentMask;
    }
    return true;
}

int checkInsert(workTable *table, Instr *instruction, InstructionPosition *pos, int *minPos)
{
    int row = pos->linearRow;
    int pipe = pos->pipelineIndex;
    int r4 = pos->loopRow;

    if (*minPos < row)
        *minPos = row;

    short e = instruction->externalInputConstraint;
    if (e < instruction->externalInputPairedConstraint)
        e = instruction->externalInputPairedConstraint;
    int minimumInputConstraint = e;
    if (r4 < minimumInputConstraint + activeSchedulingRangeStart) {
        int minimumRequiredRow = *minPos;
        int next;
        if (!activeLoopSize)
            next = minimumInputConstraint;
        else {
            int rem = minimumInputConstraint % activeLoopSize;
            int loopQuotient = minimumRequiredRow / activeLoopSize;
            if (rem <= minimumRequiredRow % activeLoopSize)
                loopQuotient++;
            next = loopQuotient * activeLoopSize + rem;
        }
        *minPos = next;
        return 0x10000;
    }

    short l = instruction->externalOutputConstraint;
    if (l < instruction->externalOutputPairedConstraint)
        l = instruction->externalOutputPairedConstraint;
    if (l && activeSchedulingRangeEnd >= 0 && activeSchedulingRangeEnd - r4 <= l) {
        *minPos = 30000;
        return 0x10000;
    }
    if (staggerMemoryAccess && instruction->ramAccessType && (bool)(r4 & 1)) {
        *minPos = row + 1;
        return 0x10000;
    }

    if (table->loopSize) {
        if (row / table->loopSize > 16) {
            *minPos = 30000;
            return 0x8000;
        }
    } else if (row >= table->rowCapacity) {
        *minPos = 30000;
        return 0x8000;
    }

    int earliestDependencyRow = 0;
    if (!instruction->hasExtraPredecessors) {
        if (!calculateEarliestDependencyRow(instruction, &earliestDependencyRow)) {
            reportError("check fdeps failed\n");
            exitAfterCleanup(-1);
            return 0x80000000;
        }
        if (earliestDependencyRow < 0) {
            *minPos = 30000;
            return 0x100000;
        }
        if (earliestDependencyRow > *minPos) {
            *minPos = earliestDependencyRow;
            return 0x100000;
        }
    }

    Instr **pipelineEntries = ((cls_unk30 *)table)->getIE_ck(pos->loopRow);
    if (pipelineEntries[pipe] != (Instr *)instruction->replacedInstructionWord) {
        *minPos = row + 1;
        return 0x40;
    }
    if (!wouldPair(pipelineEntries, instruction, pipe)) {
        (*minPos)++;
        return 0x80;
    }
    return checkLocks(instruction, &pos->linearRow, minPos);
}

bool DoInsert(workTable *const table, Instr *instruction, InstructionPosition *pos)
{
    short b = instruction->abstractMoveOpcode;
    if (__builtin_expect(b != 0, 1) && !(b = instruction->concreteMoveOpcode)) {
        reportError("undo insert currmuid not set\n");
        return false;
    }
    short r = 0;
    char p = 0;
    {
        int loopIterationHighNibble = (unsigned char)pos->loopIteration;
        loopIterationHighNibble >>= 4;
        loopIterationHighNibble >>= 4;
        if (loopIterationHighNibble)
            return r != p;
    }
    r = pos->loopRow;
    wtRow *row = &table->rows[r];
    InstructionPosition *instructionPosition = &instruction->position;
    b = pos->linearRow;
    instructionPosition->loopRow = r;
    char a = pos->loopIteration;
    instructionPosition->loopIteration = a;
    p = pos->getPipe();
    instructionPosition->linearRow = b;
    instructionPosition->pipelineIndex = p;
    row->entries[p].instruction = instruction;
    b = setLocks(instruction);
    if (!(char)b) {
        reportError("DoInsert: failed to set locks in instruction insert\n");
        return false;
    }
    int insertionCycle;
    do {
        instruction->insertionSerial = table->nextInsertionSerial;
        table->nextInsertionSerial++;
        b = UnlinkFromToDo((Block *)table, instruction);
        if (!(char)b) {
            reportError("failed to unlink from to do list\n");
            abortAfterError();
        }
        a = AddDone_Tail((Block *)table, instruction);
        if (!a) {
            reportError("failed to add to done\n");
            abortAfterError();
        }
        insertionCycle = ((unsigned char)pos->pipelineIndex | (unsigned char)pos->loopIteration);
        insertionCycle >>= 4;
        insertionCycle >>= 4;
    } while (insertionCycle);
    return true;
}

bool UndoInsert(workTable *table, Instr *instruction)
{
    if (instruction->branchFlags)
        table->loopBoundaryInstruction = 0;
    if (instruction->abstractMoveOpcode) {
        if (!instruction->concreteMoveOpcode) {
            reportError("undo insert currmuid not set\n");
            return false;
        }
        if (!unresolveMove((Block *)table, instruction)) {
            reportError("failed to unresolve move\n");
            abortAfterError();
        }
    }
    if (!table->takeOut(instruction)) {
        reportError("failed to take out instruction!\n");
        abortAfterError();
    }
    if (!undoLocks(instruction)) {
        reportError("UndoInsert:: undo locks - failed\n");
        abortAfterError();
        return false;
    }
    table->nextInsertionSerial--;
    instruction->insertionSerial = -1;
    if (!UnlinkFromDone((Block *)table, instruction)) {
        reportError("failed to unlink from done list\n");
        abortAfterError();
    }
    if (!AddToDo_Head((Block *)table, instruction)) {
        reportError("failed to relink to todo list\n");
        abortAfterError();
    }
    return true;
}

bool reduceExtents(workTable *table)
{
    setRange(table->rangeStart, table->rangeEnd, table->loopSize);
    int moved;
    do {
        moved = 0;
        for (int rowIndex = table->rangeStart; __builtin_expect(rowIndex != table->rangeEnd, 1); rowIndex++) {
            wtRow *row = table->getRowByIndex(rowIndex);
            for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                Instr *instruction = row->getEntry(pipeIndex);
                if (!instruction)
                    continue;
                int instructionRow = instruction->position.linearRow;
                int cnt = 0;
                int minEnd = 30000;
                Operand *operand;
                for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
                    RegPoolEntry *reg = registerOperand(operand, 2);
                    if (reg) {
                        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                            if ((operand->componentMask >> partIndex) & 1) {
                                cnt++;
                                OpSlot *slot = &operand->parts[partIndex];
                                if (slot->getRefCount() != 0) {
                                    int e = slot->earliestConsumerFreeCycle;
                                    if (e < minEnd)
                                        minEnd = e;
                                }
                            }
                        }
                    }
                    if (registerOperand(operand, 1))
                        cnt -= countSetBits(operand->componentMask);
                }
                if (minEnd < 30000 && __builtin_expect(cnt >= 0, 1)) {
                    int candidateRow = minEnd;
                    InstructionPosition pos;
                    bool found = false;
                    for (; candidateRow > instructionRow; candidateRow--) {
                        pos.setFromLinearRow(candidateRow);
                        if (!checkPlacementInAllowedPipelines(table, instruction, &pos, false)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        if (!table->takeOut(instruction)) {
                            reportError("failed to take out instruction\n");
                            abortAfterError();
                        }
                        if (!moveLocks(instruction, &pos)) {
                            reportError("failed to move !\n");
                            abortAfterError();
                        }
                        if (!table->putIn(instruction)) {
                            reportError("failed to put in back in\n");
                            abortAfterError();
                        }
                        moved++;
                    }
                }
            }
        }
    } while (moved);
    return true;
}

int drag(workTable *table, Instr *instruction, InstructionPosition *pos, int back, int depth)
{
    if (!table->loopSize)
        return 0;
    pullSpecialRegisterWritersEarlier(table, instruction, pos);
    pullLatencyWritersEarlier(table, instruction, pos);
    int total = 0;
    int movedLockCount;
    do {
        movedLockCount = 0;
        Operand *operand;
        for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
            RegPoolEntry *registerEntry = registerOperand(operand, 1);
            if (!registerEntry)
                continue;
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                if ((operand->componentMask >> partIndex) & 1) {
                OpSlot *slot = &operand->parts[partIndex];
                OperandLock *par = slot->getParent();
                Instr *otherInstruction = par->ownerInstr();
                if (!otherInstruction)
                    continue;
                int maxPos = pos->linearRow - par->issueLatency - slot->issueLatency;
                int start = pos->linearRow - back;
                if (!back)
                    start = otherInstruction->position.linearRow + 1;
                if (!par->latencyModeFlag)
                    start -= par->issueLatency;
                bool single = true;
                if (otherInstruction->requiresFullLoopMoveDrag) {
                    start = otherInstruction->position.linearRow + 1;
                    if (maxPos > table->loopSize + otherInstruction->position.linearRow)
                        maxPos = table->loopSize + otherInstruction->position.linearRow;
                    if (maxPos > otherInstruction->moveRowOffset)
                        maxPos = otherInstruction->moveRowOffset;
                    single = false;
                }
                InstructionPosition *opos = &otherInstruction->position;
                if (start <= opos->linearRow)
                    continue;
                if (start > maxPos)
                    continue;
                bool moved = false;
                char pipe = opos->pipelineIndex;
                InstructionPosition np;
                np.setInstructionPipe(pipe);
                np.setFromLinearRow(start);
                if (depth <= 3) {
                    int dragBackDistance = back;
                    if (otherInstruction->requiresFullLoopMoveDrag)
                        dragBackDistance = table->loopSize;
                    InstructionPosition tmp;
                    movedLockCount += drag(table, otherInstruction, &np, dragBackDistance, depth + 1);
                }
                InstructionPosition cur;
                cur.linearRow = np.linearRow;
                cur.loopRow = np.loopRow;
                cur.loopIteration = np.loopIteration;
                cur.pipelineIndex = np.pipelineIndex;
                for (; start <= maxPos; start++) {
                    cur.setFromLinearRow(start);
                    if (!checkPlacementInAllowedPipelines(table, otherInstruction, &cur, false)) {
                        np.linearRow = cur.linearRow;
                        np.loopRow = cur.loopRow;
                        np.loopIteration = cur.loopIteration;
                        np.pipelineIndex = cur.pipelineIndex;
                        moved = true;
                        if (single)
                            break;
                    }
                }
                if (moved) {
                    if (!table->takeOut(otherInstruction)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(otherInstruction, &np)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!table->putIn(otherInstruction)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                    movedLockCount++;
                }
                }
            }
        }
        total += movedLockCount;
    } while (movedLockCount);
    return total;
}

RegisterPartLockList *workTable::getLockList(RegPoolEntry *reg, int part)
{
    RegisterPartLockList *lockList = findLockList(registerLockLists, reg, part);
    if (lockList)
        return lockList;
    lockList = new RegisterPartLockList(registerLockLists);
    if (!lockList) {
        reportError("failed to get new locklist\n");
        abortAfterError();
    }
    registerLockListCount++;
    registerLockLists = lockList;
    if (!lockList->setRepReg(reg)) {
        reportError("failed to set representative reg\n");
        return 0;
    }
    lockList->componentIndex = part;
    return lockList;
}

int checkInstructionPlacementConstraints(workTable *table, Instr *instruction, InstructionPosition *pos)
{
    int row = pos->linearRow;
    int pipe = pos->pipelineIndex;
    int r4 = pos->loopRow;

    short e = instruction->externalInputConstraint;
    if (e < instruction->externalInputPairedConstraint)
        e = instruction->externalInputPairedConstraint;
    if (r4 < e + activeSchedulingRangeStart)
        return 0x10000;

    short l = instruction->externalOutputConstraint;
    if (l < instruction->externalOutputPairedConstraint)
        l = instruction->externalOutputPairedConstraint;
    if (l && activeSchedulingRangeEnd >= 0 && activeSchedulingRangeEnd - r4 <= l)
        return 0x10000;
    if (staggerMemoryAccess && instruction->ramAccessType && (bool)(r4 & 1))
        return 0x10000;

    if (table->loopSize) {
        if (row / table->loopSize > 16)
            return 0x8000;
    } else if (row >= table->rowCapacity) {
        return 0x8000;
    }

    if (!instruction->hasExtraPredecessors) {
        int earliestDependencyRow = 0;
        if (!calculateEarliestDependencyRow(instruction, &earliestDependencyRow)) {
            reportError("check fdeps failed\n");
            exitAfterCleanup(-1);
            return 0x80000000;
        }
        if (earliestDependencyRow < 0 || earliestDependencyRow > row)
            return 0x100000;
    }
    int latestDependencyRow = 30000;
    if (!calculateLatestDependencyRow(instruction, &latestDependencyRow)) {
        reportError("check fdeps failed\n");
        exitAfterCleanup(-1);
        return 0x80000000;
    }
    if (latestDependencyRow <= row)
        return 0x100000;

    Instr **pipelineEntries = ((cls_unk30 *)table)->getIE_ck(pos->loopRow);
    if (pipelineEntries[pipe] != (Instr *)instruction->replacedInstructionWord && pipelineEntries[pipe] != instruction)
        return 0x40;
    if (!wouldPair(pipelineEntries, instruction, pipe))
        return 0x80;
    return canMoveLocks(instruction, &pos->linearRow);
}

int pullSpecialRegisterWritersEarlier(workTable *table, Instr *instruction, InstructionPosition *pos)
{
    int row = instruction->position.getRow() < 0 ? pos->getRow() : instruction->position.getRow();
    if (row < 0) {
        reportError("inst not inserted\n");
        abortAfterError();
    }
    Operand *operand;
    for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (!reg)
            continue;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((operand->componentMask >> partIndex) & 1) {
                OpSlot *slot = &operand->parts[partIndex];
                OperandLock *par = slot->getParent();
                int idx = reg->registerIndex;
                if (idx != 48 && idx != 49 && idx != 59)
                    continue;
                Instr *otherInstruction = par->ownerInstr();
                if (!otherInstruction)
                    continue;
                int targetRow = instruction->position.linearRow - par->issueLatency - slot->issueLatency;
                int oiRow = otherInstruction->position.linearRow;
                if (targetRow <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(targetRow);
                for (; oiRow < targetRow; targetRow--) {
                    trial.setFromLinearRow(targetRow);
                    if (!checkPlacementInAllowedPipelines(table, otherInstruction, &trial, true)) {
                        dest.linearRow = trial.linearRow;
                        dest.loopRow = trial.loopRow;
                        dest.loopIteration = trial.loopIteration;
                        dest.pipelineIndex = trial.pipelineIndex;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (!table->takeOut(otherInstruction)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(otherInstruction, &dest)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!table->putIn(otherInstruction)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                }
            }
        }
    }
    return 1;
}

int checkPlacementInAllowedPipelines(workTable *table, Instr *instruction, InstructionPosition *pos, bool upwards)
{
    int result = 0;
    if (instruction->permittedPipelineMask == 3) {
        if (upwards) {
            for (int i = 0; i != 2; i++) {
                pos->setInstructionPipe(i);
                int placementFlags = checkInstructionPlacementConstraints(table, instruction, pos);
                if (!placementFlags)
                    return 0;
                result |= placementFlags;
            }
        } else {
            for (int i = 1; i >= 0; i--) {
                pos->setInstructionPipe(i);
                int r = checkInstructionPlacementConstraints(table, instruction, pos);
                if (!r)
                    return 0;
                result |= r;
            }
        }
        return result;
    }
    pos->setInstructionPipe(instruction->position.getPipe());
    return checkInstructionPlacementConstraints(table, instruction, pos);
}

int pullLatencyWritersEarlier(workTable *table, Instr *instruction, InstructionPosition *pos)
{
    int row = instruction->position.getRow() < 0 ? pos->getRow() : instruction->position.getRow();
    if (row < 0) {
        reportError("inst not inserted\n");
        abortAfterError();
    }
    Operand *operand;
    for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (!reg)
            continue;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((operand->componentMask >> partIndex) & 1) {
                OpSlot *slot = &operand->parts[partIndex];
                OperandLock *par = slot->getParent();
                if (par->activeReferenceCount != 0)
                    continue;
                Instr *otherInstruction = par->ownerInstr();
                if (!otherInstruction)
                    continue;
                if (otherInstruction->ramAccessType != 1 && otherInstruction->schedulingStatistics.forwardLatencyDepth != 0)
                    continue;
                int targetRow = instruction->position.linearRow - par->issueLatency - slot->issueLatency;
                int oiRow = otherInstruction->position.linearRow;
                if (targetRow <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(targetRow);
                for (; oiRow < targetRow; targetRow--) {
                    trial.setFromLinearRow(targetRow);
                    if (!checkPlacementInAllowedPipelines(table, otherInstruction, &trial, true)) {
                        dest.linearRow = trial.linearRow;
                        dest.loopRow = trial.loopRow;
                        dest.loopIteration = trial.loopIteration;
                        dest.pipelineIndex = trial.pipelineIndex;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (!table->takeOut(otherInstruction)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(otherInstruction, &dest)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!table->putIn(otherInstruction)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                }
            }
        }
    }
    return 1;
}
