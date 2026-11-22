#include "wrangle_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;


bool scheduleMidTableMode0(workTable *table, wrangleFunc candidateSelector)
{
    allowInputLockLoopWrapping = 1;
    table->unk_ad = 0;
    table->fallbackSchedulingActive = 0;
    fatalReadBeforeReady = 1;
    table->loopBoundaryInstruction = 0;
    table->recordedMaximumBranchLoopId = -1;
    table->fallbackAdjustedLoopLimit = 0;
    table->fallbackAbsoluteLoopLimit = 0;
    table->vectorMoveCount = 0;
    table->oddPipeMoveCount = 0;
    table->evenPipeMoveCount = 0;
    for (Instr *instruction = table->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        adjustMoveCount((Block *)table, instruction, 1);
        if (!instruction->classifySchedulingCategory()) {
            reportError("failed to set loop sfx type\n");
            abortAfterError();
        }
    }
    if (table->loopSize) {
        int row = table->rangeStart;
        if (row != 0 || table->rangeEnd != table->loopSize) {
            row = table->loopSize;
            reportError("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          table->rangeStart, table->rangeEnd, row);
            return false;
        }
    } else {
        int row = table->rangeStart;
        if (row != 0 || table->rangeEnd >= 0) {
            row = table->loopSize;
            reportError("wrangle - non loop set up correctly [%d,%d)\n", table->rangeStart, table->rangeEnd);
            return false;
        }
    }
    if (!candidateSelector) {
        reportError("Wrangle: passed a bad func\n");
        return false;
    }
    earliestInsertionInstruction = 0;
    if (table->loopSize)
        mode0WatchdogSeconds = table->pendingInstructions.count / 30 + 6;
    else
        mode0WatchdogSeconds = table->pendingInstructions.count / 5 + 4;
    if (mode0WatchdogSeconds < optimizationTimeoutSeconds)
        mode0WatchdogSeconds = optimizationTimeoutSeconds;
    if (!setWatchdog((char *)&mode0WatchdogExpired, mode0WatchdogSeconds)) {
        reportError("failed to set OS_setWatchdog\n");
        return false;
    }
    bool schedulingSucceeded = wrangleMode0Worker(table, candidateSelector);
    if (!clearWatchdog()) {
        reportError("failed to clear watchdog timer\n");
        return false;
    }
    return schedulingSucceeded;
}

bool wrangleMode0Worker(workTable *table, wrangleFunc candidateSelector)
{
    InstructionPosition pos;
    for (;;) {
        Instr *pending;
        VclPtrWord tmp;
        Instr *head;
        wrangleCand selectedCandidate;
        int minPos;
        wrangleCand mor;
        if (mode0WatchdogExpired) {
            reportWarning("time out..\n");
            return false;
        }
    rebuild:
        if (!buildCandidateList2(table)) {
            reportError("Wrangle: failed to build candidate list!\n");
            return false;
        }
        if (!table->moveListHead && !table->pendingInstructions.head) {
            table->rangeStart = 0;
            if (!table->loopSize) {
                int currentDoneSize = table->getCurrentDoneSize();
                if (currentDoneSize < 0) {
                    reportError("error getting current done size\n");
                    return false;
                }
                table->rangeEnd = currentDoneSize;
            } else {
                table->rangeEnd = table->loopSize;
                int maxLid = 0;
                int maxBranchLid = 0;
                int minSfx2 = 0;
                int minSfx1 = 0;
                if (!table->getCurrentLidStats(&maxLid, &maxBranchLid, &minSfx1, &minSfx2)) {
                    reportError("failed to lid stats ?\n");
                    return false;
                }
                bool b2 = minSfx2 >= 0 && minSfx2 < maxBranchLid;
                bool b1 = minSfx1 >= 0 && minSfx1 < maxBranchLid - table->minimumSafeLoopCount;
                if (b2 || b1) {
                    table->recordedMaximumBranchLoopId = maxBranchLid;
                    int lim2 = maxBranchLid * table->loopSize;
                    int lim1 = (maxBranchLid - table->minimumSafeLoopCount) * table->loopSize;
                    earliestInsertionInstruction = 0;
                    for (Instr *instruction = table->completedInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
                        switch ((unsigned char)instruction->schedulerCategory) {
                        case 0:
                            break;
                        case 1:
                            if (instruction->position.linearRow < lim1)
                                trackEarliestInsertion(instruction);
                            break;
                        case 2:
                            if (instruction->position.linearRow < lim2) {
                                int saved = errorOccurred;
                                errorOccurred = !quietDiagnostics;
                                instruction->leDump();
                                errorOccurred = saved;
                                trackEarliestInsertion(instruction);
                            }
                            break;
                        default:
                            reportError("bad case in sfx type\n");
                            exitAfterCleanup(-1);
                        }
                    }
                    if (!earliestInsertionInstruction) {
                        reportError("no fInst set after unwinding from bad side effects\n");
                        exitAfterCleanup(-1);
                    }
                    reportInfo("sidefx : loop@ %d critical at %d recoverable at %d minSafe=%d..so abort\n",
                                  maxBranchLid, minSfx2, minSfx1, (int)table->minimumSafeLoopCount);
                    table->moveListHead = 0;
                    goto zero_after_pending;
                }
            }
            if (!table->changeMode(3)) {
                reportError("failed to change code mode to raw\n");
                abortAfterError();
            }
            return true;
        }
        {
            if (table->completedInstructions.last())
                table->lastScheduledPosition.setFromLinearRow(table->completedInstructions.last()->position.getRow());
            else
                table->lastScheduledPosition.setFromLinearRow(0);
        }
        pending = 0;
        while (*(&table->moveListHead)) {
            if (!candidateSelector(table, (int *)&selectedCandidate)) {
                head = table->moveListHead = 0;
                goto test_after_insert;
            }
            {
                tmp = table->loopSize;
                bool isLoop = tmp != 0;
                int lsz = tmp;
                if (isLoop && selectedCandidate.candidateInstruction->abstractMoveOpcode) {
                    int pipe = selectedCandidate.candidatePipe;
                    if (selectedCandidate.candidateInstruction->permittedPipelineMask == 3) {
                        if (table->oddPipeMoveCount == tmp)
                            pipe = 1;
                        else if (table->evenPipeMoveCount == tmp)
                            pipe = 0;
                        else if (table->fallbackSchedulingActive)
                            pipe = 0;
                        else
                            pipe = (tmp = table->oddPipeMoveCount > table->evenPipeMoveCount);
                    }
                    if (pipe == 1 && table->evenPipeMoveCount == lsz && !table->fallbackSchedulingActive)
                        goto switch_pipe;
                    if (!resolveMovePipe(table, selectedCandidate.candidateInstruction, (tmp = pipe))) {
                        reportError("failed to set up a move to a pipe\n");
                        return false;
                    }
                }
            }
            if (refineMode0InsertionCandidate(table, &selectedCandidate)) {
                if (!selectedCandidate.candidateInstruction)
                    exitAfterCleanup(-1);
                pos.setFromLinearRow(selectedCandidate.candidateRow);
                pos.setInstructionPipe(selectedCandidate.candidatePipe);
                minPos = selectedCandidate.candidateRow;
                earliestInsertionInstruction = 0;
                if (table->loopSize) {
                    Instr *dragInst = selectedCandidate.candidateInstruction;
                    if (!dragInst->requiresFullLoopMoveDrag)
                        drag(table, dragInst, &pos, table->loopSize >> 1, 0);
                    else
                        drag(table, dragInst, &pos, table->loopSize, 0);
                } else {
                    pullSpecialRegisterWritersEarlier(table, selectedCandidate.candidateInstruction, &pos);
                    pullLatencyWritersEarlier(table, selectedCandidate.candidateInstruction, &pos);
                }
                earliestInsertionInstruction = 0;
                int ret = checkInsert(table, selectedCandidate.candidateInstruction, &pos, &minPos);
                if (earliestInsertionInstruction)
                    goto faulted;
                if (ret & 0x100) {
                    int dragResult = drag(table, selectedCandidate.candidateInstruction, &pos, table->loopSize, 4);
                    if (dragResult < 0) {
                        reportError("failed to drag setters\n");
                        return false;
                    }
                    int z1 = 1;
                    Instr **sourceInstructions;
                    if (dragResult) {
                        ret = checkInsert(table, selectedCandidate.candidateInstruction, &pos, &minPos);
                        sourceInstructions = &pending;
                    } else {
                        sourceInstructions = &pending;
                    }
                    int z2 = z1 - 1;
                    if (z2)
                        *sourceInstructions = 0;
                    if ((ret & 0x100) && !pending) {
                        tmp = (VclPtrWord)selectedCandidate.candidateInstruction;
                        pending = (Instr *)tmp;
                        mor = selectedCandidate;
                        goto check_pending;
                    }
                }
                if (!ret) {
                    if (selectedCandidate.candidateInstruction->abstractMoveOpcode && !table->loopSize) {
                        if (!resolveMovePipe(table, selectedCandidate.candidateInstruction, selectedCandidate.candidatePipe)) {
                            reportError("failed to assign a move to a pipe\n");
                            exitAfterCleanup(-1);
                        }
                    }
                    if (DoInsert(table, selectedCandidate.candidateInstruction, &pos))
                        goto after_insert_test;
                    reportError("was ok to insert..but failed!\n");
                    table->dump();
                    exitAfterCleanup(-1);
                }
            }
            {
                Instr *prev = 0, *currentInstruction;
                for (currentInstruction = table->moveListHead; currentInstruction; prev = currentInstruction, currentInstruction = currentInstruction->nextMoveInstruction)
                    if (currentInstruction == (Instr *)(tmp = (VclPtrWord)selectedCandidate.candidateInstruction))
                        break;
                if (currentInstruction != selectedCandidate.candidateInstruction) {
                    reportError("instruction wasn't on candidate list\n");
                    exitAfterCleanup(-1);
                }
                Instr *const *candidateSlot = &selectedCandidate.candidateInstruction;
                table->pendingMoveCount--;
                if (prev)
                    prev->nextMoveInstruction = (*candidateSlot)->nextMoveInstruction;
                else
                    table->moveListHead = (*candidateSlot)->nextMoveInstruction;
            }
        }
        goto check_pending;
    after_insert_test:
        head = table->moveListHead;
    test_after_insert:
        if (head)
            goto next;
    check_pending:
        if (pending) {
            selectedCandidate = mor;
            int moveResult = getMove((Block *)table, selectedCandidate.candidateInstruction, selectedCandidate.candidateRow);
            if (moveResult < 0) {
                reportError("oh no genmove failed!\n");
                table->dump();
                exitAfterCleanup(-1);
            }
            if (!moveResult)
                goto rebuild;
            if (moveResult == 4)
                reportWarning("move wasn't needed - so try to code this\n");
            if (table->unk_ad && !table->fallbackSchedulingActive)
                goto switch_pipe;
            if (moveResult == 1) {
                Instr *firstInstruction = earliestInsertionInstruction;
                table->moveListHead = 0;
                if (firstInstruction)
                    goto zero_after_pending;
                reportWarning("finst not set for no slots\n");
            }
        }
    after_pending_test:
        head = table->moveListHead;
    test_after_pending:
        if (head)
            goto next;
        goto undo;
    switch_pipe:
        table->fallbackSchedulingActive = 1;
        table->unk_ad = 0;
        earliestInsertionInstruction = table->completedInstructions.head;
    faulted:
        table->moveListHead = 0;
    zero_after_pending:
        head = 0;
        goto test_after_pending;
    undo:
        {
            Instr *firstInstruction;
            if (earliestInsertionInstruction)
                firstInstruction = earliestInsertionInstruction;
            else
                firstInstruction = earliestInsertionInstruction = table->completedInstructions.last();
            while (table->completedInstructions.last() != firstInstruction) {
                Instr *instruction = table->completedInstructions.last();
                instruction->clearTransientFlags();
                if (!UndoInsert(table, instruction)) {
                    reportError("in finst track..undoinsert failed\n");
                    exitAfterCleanup(-1);
                    return false;
                }
                if (instruction->moveRecordWord) {
                    if (!undoMovesTo((Block *)table, instruction)) {
                        reportError("failed to undo move\n");
                        exitAfterCleanup(-1);
                    }
                    firstInstruction = earliestInsertionInstruction;
                } else {
                    firstInstruction = earliestInsertionInstruction;
                }
            }
            while (table->completedInstructions.first()) {
                Instr *instruction = table->completedInstructions.last();
                if (!UndoInsert(table, instruction)) {
                    reportError("undo insert: failed to remove done list tail\n");
                    exitAfterCleanup(-1);
                    return false;
                }
                if (++instruction->candidateAttemptCount <= instruction->candidateAttemptLimit)
                    break;
                instruction->clearTransientFlags();
                int moveRecordWord = instruction->moveRecordWord;
                if (instruction) {
                    instruction->candidateAttemptCount = 0;
                    instruction->candidateAttemptLimit++;
                    int cap;
                    if (table->loopSize && instruction->candidateAttemptLimit > table->loopSize * 16) {
                        if (moveRecordWord) {
                            if (!undoMovesTo((Block *)table, instruction)) {
                                reportError("failed to undo move\n");
                                exitAfterCleanup(-1);
                            }
                            instruction = 0;
                        }
                        if (!instruction)
                            continue;
                        cap = table->loopSize * 16;
                    } else {
                        if (instruction->candidateAttemptLimit <= table->maximumDynamicStartRow * 8)
                            continue;
                        cap = table->maximumDynamicStartRow * 8;
                    }
                    instruction->candidateAttemptLimit = cap;
                }
            }
            earliestInsertionInstruction = 0;
        }
    next:;
    }
}

static inline void clearCand(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

bool buildCandidateList2(workTable *table)
{
    Instr *instruction;
    instruction = table->pendingInstructions.head;
    table->moveListHead = 0;
    table->pendingMoveCount = 0;
    InstructionPosition pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    dynCalcStats(table);
    for (; instruction; instruction = instruction->pendingLink.nextInstruction) {
        short inputConstraint = instruction->externalInputConstraint;
        if (inputConstraint < instruction->externalInputPairedConstraint)
            inputConstraint = instruction->externalInputPairedConstraint;
        int minRow = inputConstraint;
        earliest = 0;
        if (!calculateEarliestDependencyRow(instruction, &earliest)) {
            reportError("failed check cand\n");
            exitAfterCleanup(-1);
        }
        if (earliest < 0) {
            clearCand(instruction);
            continue;
        }
        if (!instruction->hasExtraPredecessors && minRow <= earliest)
            minRow = earliest;
        if (instruction->branchFlags) {
            if (activeLoopSize) {
                minRow = activeLoopSize - 2;
                if (minRow < 0) {
                    reportError("branch at bad pos for loop\n");
                    exitAfterCleanup(-1);
                }
            } else {
                if (table->pendingInstructions.count > 1)
                    continue;
                int currentDoneSize = table->getCurrentDoneSize();
                if (currentDoneSize < 0) {
                    reportError("failed to get current done size\n");
                    return false;
                }
                currentDoneSize -= 2;
                if (minRow < currentDoneSize)
                    minRow = currentDoneSize;
                if (minRow < 0)
                    minRow = 0;
            }
        }
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegPoolEntry *registerEntry = operand->assignedRegister;
            if (!registerEntry)
                continue;
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                if ((operand->componentMask >> partIndex) & 1) {
                    if (isRegisterOperand(operand, 1)) {
                        OpSlot *slot = &operand->parts[partIndex];
                        OperandLock *operandLock = slot->getParent();
                        if (!operandLock) {
                            reportError("input had no link\n");
                            exitAfterCleanup(-1);
                            return false;
                        }
                        if (!operandLock->insertedIntoLockList)
                            goto next;
                        if (slot->operandStatusCode != 2) {
                            int readyRow = operandLock->linearRow + operandLock->issueLatency + slot->issueLatency;
                            if (readyRow > minRow)
                                minRow = readyRow;
                        }
                    }
                }
            }
            if (registerEntry->moveInstruction && instruction != registerEntry->moveInstruction && registerEntry->moveInstruction->position.loopRow < 0)
                goto next;
        }
        if (table->loopSize) {
            int type = (unsigned char)instruction->schedulerCategory;
            int lim = 0;
            switch (type) {
            case 0:
                break;
            case 1:
                if (table->loopBoundaryInstruction)
                    lim = (table->loopBoundaryInstruction->position.loopIteration - table->minimumSafeLoopCount) * table->loopSize;
                else
                    lim = table->fallbackAdjustedLoopLimit;
                break;
            case 2:
                if (table->loopBoundaryInstruction)
                    lim = table->loopBoundaryInstruction->position.loopIteration * table->loopSize;
                else
                    lim = table->fallbackAbsoluteLoopLimit;
                break;
            }
            if (minRow < lim)
                minRow = lim;
            if (instruction->schedulingStatistics.reverseLatencyDepth && minRow < instruction->dynamicReverseStart - 2 * table->loopSize)
                minRow = instruction->dynamicReverseStart - 2 * table->loopSize;
        }
        clearCand(instruction);
        row = minRow;
        pipe = 0;
        if (findInsertPositionMode0(table, instruction, &row, &pipe)) {
            instruction->candidateBaseRow = row;
            if (!queueMoveForResolution((Block *)table, instruction)) {
                reportError("failed to add candidate\n");
                return false;
            }
        }
        if (!table->loopSize && table->pendingMoveCount > 4)
            break;
    next:;
    }
    return true;
}

bool refineMode0InsertionCandidate(workTable *table, wrangleCand *candidate)
{
    int pipe;
    int row = candidate->candidateRow;
    Instr *instruction = candidate->candidateInstruction;
    pipe = 0;
    if (instruction->branchFlags)
        row = instruction->candidateBaseRow;
    if (instruction->permittedPipelineMask == 3)
        pipe = candidate->candidatePipe & 1;
    else
        pipe = instruction->permittedPipelineMask != 1;
    if (findInsertPositionMode0(table, instruction, &row, &pipe)) {
        candidate->candidateRow = row;
        candidate->candidatePipe = pipe;
        return true;
    }
    return false;
}

bool findInsertPositionMode0(workTable *table, Instr *instruction, int *row, int *pipe)
{
    bool both = false;
    if (instruction->permittedPipelineMask == 3)
        both = true;
    InstructionPosition pos;
    if (both) {
        int pipeIndex;
        int best;
        int probeRow;
        for (;;) {
            best = 30000;
            pipeIndex = *pipe;
            for (; pipeIndex <= 1; pipeIndex++) {
                probeRow = *row;
                pos.setInstructionPipe(pipeIndex);
                pos.setFromLinearRow(probeRow);
                if (checkInsert(table, instruction, &pos, &probeRow) & ~0x100) {
                    if (probeRow < best)
                        best = probeRow;
                } else {
                    *row = probeRow;
                    *pipe = pipeIndex;
                    return true;
                }
            }
            if (best >= 30000)
                return false;
            if (best == *row) {
                reportError("min_nl was nl and all failed .. %d->%d\n", best, best);
                exitAfterCleanup(-1);
            }
            *row = best;
            *pipe = 0;
        }
    } else {
        *pipe = instruction->permittedPipelineMask != 1;
        pos.setFromLinearRow(*row);
        pos.setInstructionPipe(*pipe);
        while (checkInsert(table, instruction, &pos, row) & ~0x100) {
            if (*row >= 30000)
                return false;
            pos.setFromLinearRow(*row);
        }
        if (*row >= 30000)
            return false;
    }
    return true;
}
