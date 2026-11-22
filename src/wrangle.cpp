#include "wrangle_shared.h"
#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;

int g_080de74c;
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

bool scheduleMidTableMode1(workTable *wt, wrangleFunc f)
{
    g_080de288 = 0;
    if (wt->codeModeValue != 2) {
        reportError("can't wrangle a non MID table\n");
        abortAfterError();
    }
    g_080de74c = 0;
    wt->unk_ad = 0;
    wt->unk_ac = 0;
    fatalReadBeforeReady = 1;
    wt->unk_84 = 0;
    wt->unk_7c = -1;
    wt->unk_80 = 0;
    wt->unk_82 = 0;
    wt->vectorMoveCount = 0;
    wt->oddPipeMoveCount = 0;
    wt->evenPipeMoveCount = 0;
    for (Instr *in = wt->pendingInstructions.head; in; in = in->pendingLink.nextInstruction) {
        adjustMoveCount((Block *)wt, in, 1);
        if (!in->classifySchedulingCategory()) {
            reportError("failed to set loop sfx type\n");
            abortAfterError();
        }
    }
    if (wt->loopSize) {
        int row = wt->rangeStart;
        if (row != 0 || wt->rangeEnd != wt->loopSize) {
            row = false;
            reportError("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          wt->rangeStart, wt->rangeEnd, wt->loopSize);
            return false;
        }
    } else {
        int row = wt->rangeStart;
        if (row != 0 || wt->rangeEnd >= 0) {
            row = false;
            reportError("wrangle - non loop set up correctly [%d,%d)\n", wt->rangeStart, wt->rangeEnd);
            return false;
        }
    }
    if (!f) {
        reportError("Wrangle: passed a bad func\n");
        return false;
    }
    earliestInsertionInstruction = 0;
    if (wt->loopSize)
        mode1WatchdogSeconds = wt->pendingInstructions.count / 40 + 3;
    else
        mode1WatchdogSeconds = wt->pendingInstructions.count / 5 + 1;
    if (mode1WatchdogSeconds < optimizationTimeoutSeconds)
        mode1WatchdogSeconds = optimizationTimeoutSeconds;
    if (!setWatchdog((char *)&mode1WatchdogExpired, mode1WatchdogSeconds)) {
        reportError("failed to set OS_setWatchdog\n");
        return false;
    }
    bool r = wrangleMode1Worker(wt, f);
    if (!clearWatchdog()) {
        reportError("failed to clear watchdog timer\n");
        return false;
    }
    return r;
}

bool wrangleMode1Worker(workTable *wt, wrangleFunc f)
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
        Instr *m;
        int tmp;
        bool inserted = false;
        earliestInsertionInstruction = 0;
        pending = 0;
        if (!buildCandidateList(wt)) {
            reportError("Wrangle: failed to build candidate list!\n");
            abortAfterError();
        }
        wtInstrList *done = &wt->completedInstructions;
        if (done->last())
            wt->lastScheduledPosition.setFromLinearRow(done->last()->position.getRow());
        else
            wt->lastScheduledPosition.setFromLinearRow(0);
        if (!wt->moveListHead) {
            Instr *todo = wt->pendingInstructions.head;
            if (!todo) {
                wt->rangeStart = 0;
                if (!wt->loopSize) {
                    int sz = wt->getCurrentDoneSize();
                    if (sz < 0) {
                        reportError("error getting current done size\n");
                        abortAfterError();
                        return false;
                    }
                    wt->rangeEnd = sz;
                } else {
                    wt->rangeEnd = wt->loopSize;
                    int maxLid = 0;
                    int maxBranchLid = 0;
                    int minSfx2 = 0;
                    int minSfx1 = 0;
                    if (!wt->getCurrentLidStats(&maxLid, &maxBranchLid, &minSfx1, &minSfx2)) {
                        reportError("failed to lid stats ?\n");
                        return false;
                    }
                    bool b2 = minSfx2 >= 0 && minSfx2 < maxBranchLid;
                    bool b1 = minSfx1 >= 0 && minSfx1 < maxBranchLid - wt->unk_7a;
                    if (b2 || b1) {
                        wt->unk_7c = maxBranchLid;
                        int lim2 = maxBranchLid * wt->loopSize;
                        int lim1 = (maxBranchLid - wt->unk_7a) * wt->loopSize;
                        earliestInsertionInstruction = 0;
                        for (Instr *in = wt->completedInstructions.head; in; in = in->pendingLink.nextInstruction) {
                            switch ((unsigned char)in->schedulerCategory) {
                            case 0:
                                break;
                            case 1:
                                if (in->position.linearRow < lim1)
                                    trackEarliestInsertion(in);
                                break;
                            case 2:
                                if (in->position.linearRow < lim2) {
                                    int saved = errorOccurred;
                                    errorOccurred = !quietDiagnostics;
                                    in->leDump();
                                    errorOccurred = saved;
                                    trackEarliestInsertion(in);
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
                if (!wt->changeMode(3)) {
                    reportError("failed to change code mode to raw\n");
                    abortAfterError();
                }
                return true;
            } else {
                wt->dump();
                bFaulted = true;
                wt->moveListHead = 0;
                goto check_faulted;
            }
        } else {
            wrangleCand c;
            c.candidateInstruction = 0;
            c.candidateRow = -1;
            c.candidatePipe = 0;
            wrangleCand mor;
            mor.candidateInstruction = 0;
            mor.candidateRow = -1;
            mor.candidatePipe = 0;
            if (!f(wt, (int *)&c))
                goto faulted;
            tmp = wt->loopSize;
            bool isLoop = tmp != 0;
            int lsz = tmp;
            if (isLoop && (m = c.candidateInstruction)->abstractMoveOpcode) {
                int pipe = c.candidatePipe;
                if (m->permittedPipelineMask == 3) {
                    if (wt->oddPipeMoveCount == tmp)
                        pipe = 1;
                    else if (wt->evenPipeMoveCount == tmp)
                        pipe = 0;
                    else if (wt->unk_ac)
                        pipe = 0;
                    else
                        pipe = (tmp = wt->oddPipeMoveCount > wt->evenPipeMoveCount);
                }
                if (pipe == 1 && wt->evenPipeMoveCount == lsz && !wt->unk_ac) {
                    wt->unk_ac = 1;
                    wt->unk_ad = 0;
                    earliestInsertionInstruction = wt->completedInstructions.head;
                    goto faulted;
                }
                if (!resolveMovePipe(wt, c.candidateInstruction, pipe)) {
                    reportError("failed to set up a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!refineMode1InsertionCandidate(wt, &c)) {
                c.candidateRow = c.candidateInstruction->candidateBaseRow;
                if (!refineMode1InsertionCandidate(wt, &c))
                    goto faulted;
            }
            if (!c.candidateInstruction)
                exitAfterCleanup(-1);
            pos.setFromLinearRow(c.candidateRow);
            pos.setInstructionPipe(c.candidatePipe);
            int minPos = c.candidateRow;
            earliestInsertionInstruction = 0;
            if (!wt->lockLine(&pos)) {
                reportError("failed to lock a line\n");
                abortAfterError();
            }
            if (wt->loopSize) {
                Instr *in = c.candidateInstruction;
                if (!in->unk_13d1)
                    drag(wt, in, &pos, wt->loopSize >> 1, 0);
                else
                    drag(wt, in, &pos, wt->loopSize, 0);
            } else {
                pullSpecialRegisterWritersEarlier(wt, c.candidateInstruction, &pos);
                pullLatencyWritersEarlier(wt, c.candidateInstruction, &pos);
            }
            if (!wt->unlockLine(&pos)) {
                reportError("failed to unlock a line\n");
                abortAfterError();
            }
            earliestInsertionInstruction = 0;
            ret = checkInsert(wt, c.candidateInstruction, &pos, &minPos);
            Instr **ps = &pending;
            Instr **pc = &c.candidateInstruction;
            int *pg = &g_080de74c;
            if (earliestInsertionInstruction) {
                ps = &earliestInsertionInstruction;
                pc = &earliestInsertionInstruction;
                pg = &errorOccurred;
                goto faulted;
            }
            c.candidateInstruction->dumpOperandDiagnostics(false);
            int z1 = 1;
            int z2;
            if (ret & 0x100) {
                if (!wt->lockLine(&pos)) {
                    reportError("failed to lock line again\n");
                    abortAfterError();
                }
                int n = drag(wt, c.candidateInstruction, &pos, wt->loopSize, 4);
                if (!wt->unlockLine(&pos)) {
                    reportError("failed to unlock line again\n");
                    abortAfterError();
                }
                if (n < 0) {
                    reportError("failed to drag setters\n");
                    abortAfterError();
                }
                if (n)
                    ret = checkInsert(wt, c.candidateInstruction, &pos, &minPos);
                if ((ret & 0x100) && !pending) {
                    Instr *cand = c.candidateInstruction;
                    int v0 = g_080de74c;
                    *pc = cand;
                    *pg = v0;
                    pending = cand;
                    mor = c;
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
            if (z2) *ps = 0;
            if (c.candidateInstruction->abstractMoveOpcode && !wt->loopSize) {
                if (!resolveMovePipe(wt, c.candidateInstruction, c.candidatePipe)) {
                    reportError("failed to assign a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!DoInsert(wt, c.candidateInstruction, &pos)) {
                reportError("was ok to insert..but failed!\n");
                wt->dump();
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
                c = mor;
                if (!c.candidateInstruction) {
                    reportError("no instruction set in mOR\n");
                    abortAfterError();
                }
                int r = getMove((Block *)wt, c.candidateInstruction, c.candidateRow);
                switch (r) {
                case 0:
                    if (earliestInsertionInstruction) {
                        reportWarning("but fInst is now set after making a move.. - so clearing it\n");
                        earliestInsertionInstruction = 0;
                    }
                    break;
                case -1:
                default:
                    reportError("oh no genmove failed .. return code %d!\n", r);
                    wt->dump();
                    abortAfterError();
                    break;
                case 1:
                    if (wt->unk_ad && !wt->unk_ac) {
                        wt->unk_ac = 1;
                        wt->unk_ad = 0;
                        earliestInsertionInstruction = wt->completedInstructions.head;
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
                if (r)
                    goto faulted;
            }
        }
        goto check_faulted;
    faulted:
        bFaulted = true;
        wt->moveListHead = 0;
    check_faulted:
        if (bFaulted) {
            if (earliestInsertionInstruction)
                earliestInsertionInstruction->dumpOperandDiagnostics(false);
            else
                earliestInsertionInstruction = done->last();
            for (wtInstrList *tail = done;
                 (Instr *)*(VclPtrWord *)&tail->tail != earliestInsertionInstruction;
                 tail = &wt->completedInstructions) {
                Instr *in = tail->last();
                in->clearTransientFlags();
                if (!UndoInsert(wt, in)) {
                    reportError("in finst track..undoinsert failed\n");
                    abortAfterError();
                    return false;
                }
                if (in->moveRecordWord && !undoMovesTo((Block *)wt, in)) {
                    reportError("failed to undo move\n");
                    abortAfterError();
                }
            }
            while (wt->completedInstructions.head) {
                Instr *in = wt->completedInstructions.last();
                if (!UndoInsert(wt, in)) {
                    reportError("undo insert: failed to remove done list tail\n");
                    abortAfterError();
                    return false;
                }
                in->candidateCostOffset++;
                if (++in->candidateAttemptCount <= in->candidateAttemptLimit)
                    break;
                in->clearTransientFlags();
                if (in->moveRecordWord) {
                    if (!undoMovesTo((Block *)wt, in)) {
                        reportError("failed to undo move\n");
                        abortAfterError();
                    }
                    in = 0;
                }
                if (in) {
                    in->candidateAttemptCount = 0;
                    in->candidateAttemptLimit++;
                    if (wt->loopSize && in->candidateAttemptLimit > wt->loopSize * 16) {
                        if (in->moveRecordWord) {
                            if (!undoMovesTo((Block *)wt, in)) {
                                reportError("failed to undo move\n");
                                abortAfterError();
                            }
                            in = 0;
                        }
                        if (in)
                            in->candidateAttemptLimit = wt->loopSize * 16;
                    }
                }
            }
        }
    }
}


bool buildCandidateList(workTable *wt)
{
    Instr *in;
    in = wt->pendingInstructions.head;
    wt->moveListHead = 0;
    wt->pendingMoveCount = 0;
    InstructionPosition pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    wt->dump();
    dynCalcStats(wt);
    g_080de74c++;
    for (; in; in = in->pendingLink.nextInstruction) {
        short m = in->externalInputConstraint;
        if (m < in->externalInputPairedConstraint)
            m = in->externalInputPairedConstraint;
        int minRow = m;
        earliest = 0;
        if (!calculateEarliestDependencyRow(in, &earliest)) {
            reportError("failed check cand\n");
            abortAfterError();
        }
        if (earliest < 0) {
            in->dumpOperandDiagnostics(false);
            continue;
        }
        if (!in->hasExtraPredecessors && minRow < earliest)
            minRow = earliest;
        if (in->branchFlags) {
            if (activeLoopSize) {
                minRow = activeLoopSize - 2;
                if (minRow < 0) {
                    reportError("branch at bad pos for loop\n");
                    abortAfterError();
                }
            } else {
                if (wt->pendingInstructions.count > 1)
                    continue;
                int d = wt->getCurrentDoneSize();
                if (d < 0) {
                    reportError("failed to get current done size\n");
                    return false;
                }
                d -= 2;
                if (minRow < d)
                    minRow = d;
                if (minRow < 0)
                    minRow = 0;
            }
        }
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = op->assignedRegister;
            if (!r)
                continue;
            for (int j = 0; j != r->partCount; j++) {
                if ((op->componentMask >> j) & 1) {
                    if (isRegisterOperand(op, 1)) {
                        OpSlot *s = &op->parts[j];
                        OperandLock *p = s->getParent();
                        if (!p) {
                            reportError("input had no link\n");
                            abortAfterError();
                            return false;
                        }
                        if (!p->insertedIntoLockList)
                            goto next;
                        if (s->unk_19 != 2) {
                            int t = p->linearRow + p->issueLatency + s->issueLatency;
                            if (t > minRow)
                                minRow = t;
                        }
                    }
                }
            }
            if (r->moveInstruction && in != r->moveInstruction && r->moveInstruction->position.loopRow < 0)
                goto next;
        }
        if (wt->loopSize) {
            int type = (unsigned char)in->schedulerCategory;
            int lim = 0;
            switch (type) {
            case 0:
                break;
            case 1:
                if (wt->unk_84)
                    lim = (wt->unk_84->position.loopIteration - wt->unk_7a) * wt->loopSize;
                else
                    lim = wt->unk_80;
                break;
            case 2:
                if (wt->unk_84)
                    lim = wt->unk_84->position.loopIteration * wt->loopSize;
                else
                    lim = wt->unk_82;
                break;
            }
            if (minRow < lim)
                minRow = lim;
        }
        row = minRow;
        pipe = 0;
        if (findInsertPosition(wt, in, &row, &pipe)) {
            in->candidateBaseRow = row;
            if (!queueMoveForResolution((Block *)wt, in)) {
                reportError("failed to add candidate\n");
                return false;
            }
        }
        if (!wt->loopSize && wt->pendingMoveCount > 16)
            break;
    next:;
    }
    return true;
}

bool refineMode1InsertionCandidate(workTable *wt, wrangleCand *c)
{
    int pipe;
    int row = c->candidateRow;
    Instr *in = c->candidateInstruction;
    pipe = 0;
    if (in->branchFlags)
        row = in->candidateBaseRow;
    if (in->permittedPipelineMask == 3)
        pipe = c->candidatePipe & 1;
    else
        pipe = in->permittedPipelineMask != 1;
    if (findInsertPosition(wt, in, &row, &pipe)) {
        c->candidateRow = row;
        c->candidatePipe = pipe;
        return true;
    }
    return false;
}



bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe)
{
    bool both = false;
    if (in->permittedPipelineMask == 3)
        both = true;
    InstructionPosition pos;
    if (both) {
        int p;
        int best;
        int r;
        for (;;) {
            best = 30000;
            p = *pipe;
            for (; p <= 1; p++) {
                r = *row;
                pos.setInstructionPipe(p);
                pos.setFromLinearRow(r);
                if (checkInsert(wt, in, &pos, &r) & ~0x100) {
                    if (r < best)
                        best = r;
                } else {
                    *row = r;
                    *pipe = p;
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
        *pipe = in->permittedPipelineMask != 1;
        pos.setFromLinearRow(*row);
        pos.setInstructionPipe(*pipe);
        while (checkInsert(wt, in, &pos, row) & ~0x100) {
            if (*row >= 30000)
                return false;
            pos.setFromLinearRow(*row);
        }
        if (*row >= 30000)
            return false;
    }
    return true;
}
