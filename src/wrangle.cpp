#include "wrangle_shared.h"
#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;

int mode1SchedulingPassCount;
int mode1WatchdogSeconds;
volatile char mode1WatchdogExpired;
int mode0WatchdogSeconds;
volatile char mode0WatchdogExpired;

bool wrangleMode1Worker(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool calculateEarliestDependencyRow(Instr *i, int *row);
bool queueMoveForResolution(Block *blk, Instr *mv);
bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe);
bool wrangleMode0Worker(workTable *wt, wrangleFunc f);
bool findInsertPositionMode0(workTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(workTable *wt);
bool refineMode0InsertionCandidate(workTable *wt, wrangleCand *c);
int checkInsert(workTable *wt, Instr *in, InstructionPosition *pos, int *minPos);
bool buildCandidateList(workTable *wt);
bool refineMode1InsertionCandidate(workTable *wt, wrangleCand *c);
bool DoInsert(workTable *wt, Instr *in, InstructionPosition *pos);
bool UndoInsert(workTable *wt, Instr *in);
int drag(workTable *wt, Instr *in, InstructionPosition *pos, int back, int depth);
int pullSpecialRegisterWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int pullLatencyWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int getMove(Block *blk, Instr *in, int now);
bool undoMovesTo(Block *blk, Instr *stop);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);
extern int errorOccurred;
extern bool quietDiagnostics;

bool scheduleMidTableMode1(workTable *table, wrangleFunc candidateSelector)
{
    allowInputLockLoopWrapping = 0;
    if (table->codeModeValue != 2) {
        reportError("can't wrangle a non MID table\n");
        abortAfterError();
    }
    mode1SchedulingPassCount = 0;
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
            row = false;
            reportError("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          table->rangeStart, table->rangeEnd, table->loopSize);
            return false;
        }
    } else {
        int row = table->rangeStart;
        if (row != 0 || table->rangeEnd >= 0) {
            row = false;
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
        mode1WatchdogSeconds = table->pendingInstructions.count / 40 + 3;
    else
        mode1WatchdogSeconds = table->pendingInstructions.count / 5 + 1;
    if (mode1WatchdogSeconds < optimizationTimeoutSeconds)
        mode1WatchdogSeconds = optimizationTimeoutSeconds;
    if (!setWatchdog((char *)&mode1WatchdogExpired, mode1WatchdogSeconds)) {
        reportError("failed to set OS_setWatchdog\n");
        return false;
    }
    bool schedulingSucceeded = wrangleMode1Worker(table, candidateSelector);
    if (!clearWatchdog()) {
        reportError("failed to clear watchdog timer\n");
        return false;
    }
    return schedulingSucceeded;
}

bool wrangleMode1Worker(workTable *table, wrangleFunc candidateSelector)
{
    InstructionPosition pos;
    for (;;) {
        if (mode1WatchdogExpired) {
            reportWarning("time out..\n");
            return false;
        }
        bool bFaulted = false;
        Instr *pending;
        int ret;
        Instr *moveInstruction;
        int tmp;
        bool inserted = false;
        earliestInsertionInstruction = 0;
        pending = 0;
        if (!buildCandidateList(table)) {
            reportError("Wrangle: failed to build candidate list!\n");
            abortAfterError();
        }
        wtInstrList *done = &table->completedInstructions;
        if (done->last())
            table->lastScheduledPosition.setFromLinearRow(done->last()->position.getRow());
        else
            table->lastScheduledPosition.setFromLinearRow(0);
        if (!table->moveListHead) {
            Instr *todo = table->pendingInstructions.head;
            if (!todo) {
                table->rangeStart = 0;
                if (!table->loopSize) {
                    int currentDoneSize = table->getCurrentDoneSize();
                    if (currentDoneSize < 0) {
                        reportError("error getting current done size\n");
                        abortAfterError();
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
                                abortAfterError();
                            }
                        }
                        if (!earliestInsertionInstruction) {
                            reportError("no fInst set after unwinding from bad side effects\n");
                            abortAfterError();
                        }
                        goto faulted;
                    }
                }
                if (!table->changeMode(3)) {
                    reportError("failed to change code mode to raw\n");
                    abortAfterError();
                }
                return true;
            } else {
                table->dump();
                bFaulted = true;
                table->moveListHead = 0;
                goto check_faulted;
            }
        } else {
            wrangleCand selectedCandidate;
            selectedCandidate.candidateInstruction = 0;
            selectedCandidate.candidateRow = -1;
            selectedCandidate.candidatePipe = 0;
            wrangleCand mor;
            mor.candidateInstruction = 0;
            mor.candidateRow = -1;
            mor.candidatePipe = 0;
            if (!candidateSelector(table, (int *)&selectedCandidate))
                goto faulted;
            tmp = table->loopSize;
            bool isLoop = tmp != 0;
            int lsz = tmp;
            if (isLoop && (moveInstruction = selectedCandidate.candidateInstruction)->abstractMoveOpcode) {
                int pipe = selectedCandidate.candidatePipe;
                if (moveInstruction->permittedPipelineMask == 3) {
                    if (table->oddPipeMoveCount == tmp)
                        pipe = 1;
                    else if (table->evenPipeMoveCount == tmp)
                        pipe = 0;
                    else if (table->fallbackSchedulingActive)
                        pipe = 0;
                    else
                        pipe = (tmp = table->oddPipeMoveCount > table->evenPipeMoveCount);
                }
                if (pipe == 1 && table->evenPipeMoveCount == lsz && !table->fallbackSchedulingActive) {
                    table->fallbackSchedulingActive = 1;
                    table->unk_ad = 0;
                    earliestInsertionInstruction = table->completedInstructions.head;
                    goto faulted;
                }
                if (!resolveMovePipe(table, selectedCandidate.candidateInstruction, pipe)) {
                    reportError("failed to set up a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!refineMode1InsertionCandidate(table, &selectedCandidate)) {
                selectedCandidate.candidateRow = selectedCandidate.candidateInstruction->candidateBaseRow;
                if (!refineMode1InsertionCandidate(table, &selectedCandidate))
                    goto faulted;
            }
            if (!selectedCandidate.candidateInstruction)
                exitAfterCleanup(-1);
            pos.setFromLinearRow(selectedCandidate.candidateRow);
            pos.setInstructionPipe(selectedCandidate.candidatePipe);
            int minPos = selectedCandidate.candidateRow;
            earliestInsertionInstruction = 0;
            if (!table->lockLine(&pos)) {
                reportError("failed to lock a line\n");
                abortAfterError();
            }
            if (table->loopSize) {
                Instr *instruction = selectedCandidate.candidateInstruction;
                if (!instruction->requiresFullLoopMoveDrag)
                    drag(table, instruction, &pos, table->loopSize >> 1, 0);
                else
                    drag(table, instruction, &pos, table->loopSize, 0);
            } else {
                pullSpecialRegisterWritersEarlier(table, selectedCandidate.candidateInstruction, &pos);
                pullLatencyWritersEarlier(table, selectedCandidate.candidateInstruction, &pos);
            }
            if (!table->unlockLine(&pos)) {
                reportError("failed to unlock a line\n");
                abortAfterError();
            }
            earliestInsertionInstruction = 0;
            ret = checkInsert(table, selectedCandidate.candidateInstruction, &pos, &minPos);
            Instr **sourceInstructions = &pending;
            Instr **candidateInstructions = &selectedCandidate.candidateInstruction;
            int *passCounter = &mode1SchedulingPassCount;
            if (earliestInsertionInstruction) {
                sourceInstructions = &earliestInsertionInstruction;
                candidateInstructions = &earliestInsertionInstruction;
                passCounter = &errorOccurred;
                goto faulted;
            }
            selectedCandidate.candidateInstruction->dumpOperandDiagnostics(false);
            int z1 = 1;
            int z2;
            if (ret & 0x100) {
                if (!table->lockLine(&pos)) {
                    reportError("failed to lock line again\n");
                    abortAfterError();
                }
                int dragResult = drag(table, selectedCandidate.candidateInstruction, &pos, table->loopSize, 4);
                if (!table->unlockLine(&pos)) {
                    reportError("failed to unlock line again\n");
                    abortAfterError();
                }
                if (dragResult < 0) {
                    reportError("failed to drag setters\n");
                    abortAfterError();
                }
                if (dragResult)
                    ret = checkInsert(table, selectedCandidate.candidateInstruction, &pos, &minPos);
                if ((ret & 0x100) && !pending) {
                    Instr *cand = selectedCandidate.candidateInstruction;
                    int v0 = mode1SchedulingPassCount;
                    *candidateInstructions = cand;
                    *passCounter = v0;
                    pending = cand;
                    mor = selectedCandidate;
                    goto check_pending;
                }
            }
            {
                Instr *d0 = done->last();
                if (ret)
                    goto faulted;
                *(Instr **)&done->tail = d0;
            }
            tmp = 0;
            z2 = z1 - 1;
            if (z2) *sourceInstructions = 0;
            if (selectedCandidate.candidateInstruction->abstractMoveOpcode && !table->loopSize) {
                if (!resolveMovePipe(table, selectedCandidate.candidateInstruction, selectedCandidate.candidatePipe)) {
                    reportError("failed to assign a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!DoInsert(table, selectedCandidate.candidateInstruction, &pos)) {
                reportError("was ok to insert..but failed!\n");
                table->dump();
                abortAfterError();
            } else
                inserted = true;
            if (!inserted) {
                reportError("got here - but inserted not set bFaulted=%d ret=%x\n", bFaulted, ret);
                abortAfterError();
            }
        check_pending:
            if (pending) {
                if (bFaulted) {
                    reportError("bFaulted is set with a pending move candidate\n");
                    abortAfterError();
                }
                selectedCandidate = mor;
                if (!selectedCandidate.candidateInstruction) {
                    reportError("no instruction set in mOR\n");
                    abortAfterError();
                }
                int moveResult = getMove((Block *)table, selectedCandidate.candidateInstruction, selectedCandidate.candidateRow);
                switch (moveResult) {
                case 0:
                    if (earliestInsertionInstruction) {
                        reportWarning("but fInst is now set after making a move.. - so clearing it\n");
                        earliestInsertionInstruction = 0;
                    }
                    break;
                case -1:
                default:
                    reportError("oh no genmove failed .. return code %d!\n", moveResult);
                    table->dump();
                    abortAfterError();
                    break;
                case 1:
                    if (table->unk_ad && !table->fallbackSchedulingActive) {
                        table->fallbackSchedulingActive = 1;
                        table->unk_ad = 0;
                        earliestInsertionInstruction = table->completedInstructions.head;
                    }
                    break;
                case 2:
                case 3:
                    break;
                case 4:
                    reportError("a move isn't needed after all\n");
                    abortAfterError();
                    break;
                }
                if (moveResult)
                    goto faulted;
            }
        }
        goto check_faulted;
    faulted:
        bFaulted = true;
        table->moveListHead = 0;
    check_faulted:
        if (bFaulted) {
            if (earliestInsertionInstruction)
                earliestInsertionInstruction->dumpOperandDiagnostics(false);
            else
                earliestInsertionInstruction = done->last();
            for (wtInstrList *tail = done;
                 (Instr *)*(VclPtrWord *)&tail->tail != earliestInsertionInstruction;
                 tail = &table->completedInstructions) {
                Instr *instruction = tail->last();
                instruction->clearTransientFlags();
                if (!UndoInsert(table, instruction)) {
                    reportError("in finst track..undoinsert failed\n");
                    abortAfterError();
                    return false;
                }
                if (instruction->moveRecordWord && !undoMovesTo((Block *)table, instruction)) {
                    reportError("failed to undo move\n");
                    abortAfterError();
                }
            }
            while (table->completedInstructions.head) {
                Instr *instruction = table->completedInstructions.last();
                if (!UndoInsert(table, instruction)) {
                    reportError("undo insert: failed to remove done list tail\n");
                    abortAfterError();
                    return false;
                }
                instruction->candidateCostOffset++;
                if (++instruction->candidateAttemptCount <= instruction->candidateAttemptLimit)
                    break;
                instruction->clearTransientFlags();
                if (instruction->moveRecordWord) {
                    if (!undoMovesTo((Block *)table, instruction)) {
                        reportError("failed to undo move\n");
                        abortAfterError();
                    }
                    instruction = 0;
                }
                if (instruction) {
                    instruction->candidateAttemptCount = 0;
                    instruction->candidateAttemptLimit++;
                    if (table->loopSize && instruction->candidateAttemptLimit > table->loopSize * 16) {
                        if (instruction->moveRecordWord) {
                            if (!undoMovesTo((Block *)table, instruction)) {
                                reportError("failed to undo move\n");
                                abortAfterError();
                            }
                            instruction = 0;
                        }
                        if (instruction)
                            instruction->candidateAttemptLimit = table->loopSize * 16;
                    }
                }
            }
        }
    }
}


bool buildCandidateList(workTable *table)
{
    Instr *instruction;
    instruction = table->pendingInstructions.head;
    table->moveListHead = 0;
    table->pendingMoveCount = 0;
    InstructionPosition pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    table->dump();
    dynCalcStats(table);
    mode1SchedulingPassCount++;
    for (; instruction; instruction = instruction->pendingLink.nextInstruction) {
        short inputConstraint = instruction->externalInputConstraint;
        if (inputConstraint < instruction->externalInputPairedConstraint)
            inputConstraint = instruction->externalInputPairedConstraint;
        int minRow = inputConstraint;
        earliest = 0;
        if (!calculateEarliestDependencyRow(instruction, &earliest)) {
            reportError("failed check cand\n");
            abortAfterError();
        }
        if (earliest < 0) {
            instruction->dumpOperandDiagnostics(false);
            continue;
        }
        if (!instruction->hasExtraPredecessors && minRow < earliest)
            minRow = earliest;
        if (instruction->branchFlags) {
            if (activeLoopSize) {
                minRow = activeLoopSize - 2;
                if (minRow < 0) {
                    reportError("branch at bad pos for loop\n");
                    abortAfterError();
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
                            abortAfterError();
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
        }
        row = minRow;
        pipe = 0;
        if (findInsertPosition(table, instruction, &row, &pipe)) {
            instruction->candidateBaseRow = row;
            if (!queueMoveForResolution((Block *)table, instruction)) {
                reportError("failed to add candidate\n");
                return false;
            }
        }
        if (!table->loopSize && table->pendingMoveCount > 16)
            break;
    next:;
    }
    return true;
}

bool refineMode1InsertionCandidate(workTable *table, wrangleCand *candidate)
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
    if (findInsertPosition(table, instruction, &row, &pipe)) {
        candidate->candidateRow = row;
        candidate->candidatePipe = pipe;
        return true;
    }
    return false;
}



bool findInsertPosition(workTable *table, Instr *instruction, int *row, int *pipe)
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
                abortAfterError();
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
