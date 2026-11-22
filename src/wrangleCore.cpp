#include "wrangle_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;


bool func_0808061c(workTable *wt, wrangleFunc f)
{
    g_080de288 = 1;
    wt->unk_ad = 0;
    wt->unk_ac = 0;
    g_080de284 = 1;
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
            row = wt->loopSize;
            reportError("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          wt->rangeStart, wt->rangeEnd, row);
            return false;
        }
    } else {
        int row = wt->rangeStart;
        if (row != 0 || wt->rangeEnd >= 0) {
            row = wt->loopSize;
            reportError("wrangle - non loop set up correctly [%d,%d)\n", wt->rangeStart, wt->rangeEnd);
            return false;
        }
    }
    if (!f) {
        reportError("Wrangle: passed a bad func\n");
        return false;
    }
    g_080de72c = 0;
    if (wt->loopSize)
        g_080de758 = wt->pendingInstructions.count / 30 + 6;
    else
        g_080de758 = wt->pendingInstructions.count / 5 + 4;
    if (g_080de758 < timeout_080b3010)
        g_080de758 = timeout_080b3010;
    if (!setWatchdog((char *)&g_080de75c, g_080de758)) {
        reportError("failed to set OS_setWatchdog\n");
        return false;
    }
    bool r = wrangleMode0Worker(wt, f);
    if (!clearWatchdog()) {
        reportError("failed to clear watchdog timer\n");
        return false;
    }
    return r;
}

bool wrangleMode0Worker(workTable *wt, wrangleFunc f)
{
    InstructionPosition pos;
    for (;;) {
        Instr *pending;
        VclPtrWord tmp;
        Instr *head;
        wrangleCand c;
        int minPos;
        wrangleCand mor;
        if (g_080de75c) {
            reportWarning("time out..\n");
            return false;
        }
    rebuild:
        if (!buildCandidateList2(wt)) {
            reportError("Wrangle: failed to build candidate list!\n");
            return false;
        }
        if (!wt->moveListHead && !wt->pendingInstructions.head) {
            wt->rangeStart = 0;
            if (!wt->loopSize) {
                int sz = wt->getCurrentDoneSize();
                if (sz < 0) {
                    reportError("error getting current done size\n");
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
                    g_080de72c = 0;
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
                                errorOccurred = !quiet_080bbd98;
                                in->leDump();
                                errorOccurred = saved;
                                trackEarliestInsertion(in);
                            }
                            break;
                        default:
                            reportError("bad case in sfx type\n");
                            exitAfterCleanup(-1);
                        }
                    }
                    if (!g_080de72c) {
                        reportError("no fInst set after unwinding from bad side effects\n");
                        exitAfterCleanup(-1);
                    }
                    reportInfo("sidefx : loop@ %d critical at %d recoverable at %d minSafe=%d..so abort\n",
                                  maxBranchLid, minSfx2, minSfx1, (int)wt->unk_7a);
                    wt->moveListHead = 0;
                    goto zero_after_pending;
                }
            }
            if (!wt->changeMode(3)) {
                reportError("failed to change code mode to raw\n");
                abortAfterError();
            }
            return true;
        }
        {
            if (wt->completedInstructions.last())
                wt->unk_54.setFromLinearRow(wt->completedInstructions.last()->position.getRow());
            else
                wt->unk_54.setFromLinearRow(0);
        }
        pending = 0;
        while (*(&wt->moveListHead)) {
            if (!f(wt, (int *)&c)) {
                head = wt->moveListHead = 0;
                goto test_after_insert;
            }
            {
                tmp = wt->loopSize;
                bool isLoop = tmp != 0;
                int lsz = tmp;
                if (isLoop && c.unk_00->abstractMoveOpcode) {
                    int pipe = c.unk_06;
                    if (c.unk_00->permittedPipelineMask == 3) {
                        if (wt->oddPipeMoveCount == tmp)
                            pipe = 1;
                        else if (wt->evenPipeMoveCount == tmp)
                            pipe = 0;
                        else if (wt->unk_ac)
                            pipe = 0;
                        else
                            pipe = (tmp = wt->oddPipeMoveCount > wt->evenPipeMoveCount);
                    }
                    if (pipe == 1 && wt->evenPipeMoveCount == lsz && !wt->unk_ac)
                        goto switch_pipe;
                    if (!resolveMovePipe(wt, c.unk_00, (tmp = pipe))) {
                        reportError("failed to set up a move to a pipe\n");
                        return false;
                    }
                }
            }
            if (func_0808158c(wt, &c)) {
                if (!c.unk_00)
                    exitAfterCleanup(-1);
                pos.setFromLinearRow(c.unk_04);
                pos.setInstructionPipe(c.unk_06);
                minPos = c.unk_04;
                g_080de72c = 0;
                if (wt->loopSize) {
                    Instr *dragInst = c.unk_00;
                    if (!dragInst->unk_13d1)
                        drag(wt, dragInst, &pos, wt->loopSize >> 1, 0);
                    else
                        drag(wt, dragInst, &pos, wt->loopSize, 0);
                } else {
                    func_0807de00(wt, c.unk_00, &pos);
                    func_0807e1f8(wt, c.unk_00, &pos);
                }
                g_080de72c = 0;
                int ret = checkInsert(wt, c.unk_00, &pos, &minPos);
                if (g_080de72c)
                    goto faulted;
                if (ret & 0x100) {
                    int n = drag(wt, c.unk_00, &pos, wt->loopSize, 4);
                    if (n < 0) {
                        reportError("failed to drag setters\n");
                        return false;
                    }
                    int z1 = 1;
                    Instr **ps;
                    if (n) {
                        ret = checkInsert(wt, c.unk_00, &pos, &minPos);
                        ps = &pending;
                    } else {
                        ps = &pending;
                    }
                    int z2 = z1 - 1;
                    if (z2)
                        *ps = 0;
                    if ((ret & 0x100) && !pending) {
                        tmp = (VclPtrWord)c.unk_00;
                        pending = (Instr *)tmp;
                        mor = c;
                        goto check_pending;
                    }
                }
                if (!ret) {
                    if (c.unk_00->abstractMoveOpcode && !wt->loopSize) {
                        if (!resolveMovePipe(wt, c.unk_00, c.unk_06)) {
                            reportError("failed to assign a move to a pipe\n");
                            exitAfterCleanup(-1);
                        }
                    }
                    if (DoInsert(wt, c.unk_00, &pos))
                        goto after_insert_test;
                    reportError("was ok to insert..but failed!\n");
                    wt->dump();
                    exitAfterCleanup(-1);
                }
            }
            {
                Instr *prev = 0, *p;
                for (p = wt->moveListHead; p; prev = p, p = p->nextMoveInstruction)
                    if (p == (Instr *)(tmp = (VclPtrWord)c.unk_00))
                        break;
                if (p != c.unk_00) {
                    reportError("instruction wasn't on candidate list\n");
                    exitAfterCleanup(-1);
                }
                Instr *const *candidateSlot = &c.unk_00;
                wt->unk_50--;
                if (prev)
                    prev->nextMoveInstruction = (*candidateSlot)->nextMoveInstruction;
                else
                    wt->moveListHead = (*candidateSlot)->nextMoveInstruction;
            }
        }
        goto check_pending;
    after_insert_test:
        head = wt->moveListHead;
    test_after_insert:
        if (head)
            goto next;
    check_pending:
        if (pending) {
            c = mor;
            int r = getMove((Block *)wt, c.unk_00, c.unk_04);
            if (r < 0) {
                reportError("oh no genmove failed!\n");
                wt->dump();
                exitAfterCleanup(-1);
            }
            if (!r)
                goto rebuild;
            if (r == 4)
                reportWarning("move wasn't needed - so try to code this\n");
            if (wt->unk_ad && !wt->unk_ac)
                goto switch_pipe;
            if (r == 1) {
                Instr *fi = g_080de72c;
                wt->moveListHead = 0;
                if (fi)
                    goto zero_after_pending;
                reportWarning("finst not set for no slots\n");
            }
        }
    after_pending_test:
        head = wt->moveListHead;
    test_after_pending:
        if (head)
            goto next;
        goto undo;
    switch_pipe:
        wt->unk_ac = 1;
        wt->unk_ad = 0;
        g_080de72c = wt->completedInstructions.head;
    faulted:
        wt->moveListHead = 0;
    zero_after_pending:
        head = 0;
        goto test_after_pending;
    undo:
        {
            Instr *fi;
            if (g_080de72c)
                fi = g_080de72c;
            else
                fi = g_080de72c = wt->completedInstructions.last();
            while (wt->completedInstructions.last() != fi) {
                Instr *in = wt->completedInstructions.last();
                in->clearTransientFlags();
                if (!UndoInsert(wt, in)) {
                    reportError("in finst track..undoinsert failed\n");
                    exitAfterCleanup(-1);
                    return false;
                }
                if (in->moveRecordWord) {
                    if (!undoMovesTo((Block *)wt, in)) {
                        reportError("failed to undo move\n");
                        exitAfterCleanup(-1);
                    }
                    fi = g_080de72c;
                } else {
                    fi = g_080de72c;
                }
            }
            while (wt->completedInstructions.first()) {
                Instr *in = wt->completedInstructions.last();
                if (!UndoInsert(wt, in)) {
                    reportError("undo insert: failed to remove done list tail\n");
                    exitAfterCleanup(-1);
                    return false;
                }
                if (++in->candidateAttemptCount <= in->candidateAttemptLimit)
                    break;
                in->clearTransientFlags();
                int mv = in->moveRecordWord;
                if (in) {
                    in->candidateAttemptCount = 0;
                    in->candidateAttemptLimit++;
                    int cap;
                    if (wt->loopSize && in->candidateAttemptLimit > wt->loopSize * 16) {
                        if (mv) {
                            if (!undoMovesTo((Block *)wt, in)) {
                                reportError("failed to undo move\n");
                                exitAfterCleanup(-1);
                            }
                            in = 0;
                        }
                        if (!in)
                            continue;
                        cap = wt->loopSize * 16;
                    } else {
                        if (in->candidateAttemptLimit <= wt->unk_04 * 8)
                            continue;
                        cap = wt->unk_04 * 8;
                    }
                    in->candidateAttemptLimit = cap;
                }
            }
            g_080de72c = 0;
        }
    next:;
    }
}

static inline void clearCand(Instr *in)
{
    in->dumpOperandDiagnostics(false);
}

bool buildCandidateList2(workTable *wt)
{
    Instr *in;
    in = wt->pendingInstructions.head;
    wt->moveListHead = 0;
    wt->unk_50 = 0;
    InstructionPosition pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    dynCalcStats(wt);
    for (; in; in = in->pendingLink.nextInstruction) {
        short m = in->externalInputConstraint;
        if (m < in->externalInputPairedConstraint)
            m = in->externalInputPairedConstraint;
        int minRow = m;
        earliest = 0;
        if (!func_0806558c(in, &earliest)) {
            reportError("failed check cand\n");
            exitAfterCleanup(-1);
        }
        if (earliest < 0) {
            clearCand(in);
            continue;
        }
        if (!in->hasExtraPredecessors && minRow <= earliest)
            minRow = earliest;
        if (in->branchFlags) {
            if (g_080b336c) {
                minRow = g_080b336c - 2;
                if (minRow < 0) {
                    reportError("branch at bad pos for loop\n");
                    exitAfterCleanup(-1);
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
                            exitAfterCleanup(-1);
                            return false;
                        }
                        if (!p->unk_2c)
                            goto next;
                        if (s->unk_19 != 2) {
                            int t = p->unk_32 + p->issueLatency + s->issueLatency;
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
            if (in->schedulingStatistics.reverseLatencyDepth && minRow < in->dynamicReverseStart - 2 * wt->loopSize)
                minRow = in->dynamicReverseStart - 2 * wt->loopSize;
        }
        clearCand(in);
        row = minRow;
        pipe = 0;
        if (findInsertPositionMode0(wt, in, &row, &pipe)) {
            in->candidateBaseRow = row;
            if (!func_0807f2c4((Block *)wt, in)) {
                reportError("failed to add candidate\n");
                return false;
            }
        }
        if (!wt->loopSize && wt->unk_50 > 4)
            break;
    next:;
    }
    return true;
}

bool func_0808158c(workTable *wt, wrangleCand *c)
{
    int pipe;
    int row = c->unk_04;
    Instr *in = c->unk_00;
    pipe = 0;
    if (in->branchFlags)
        row = in->candidateBaseRow;
    if (in->permittedPipelineMask == 3)
        pipe = c->unk_06 & 1;
    else
        pipe = in->permittedPipelineMask != 1;
    if (findInsertPositionMode0(wt, in, &row, &pipe)) {
        c->unk_04 = row;
        c->unk_06 = pipe;
        return true;
    }
    return false;
}

bool findInsertPositionMode0(workTable *wt, Instr *in, int *row, int *pipe)
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
                exitAfterCleanup(-1);
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
