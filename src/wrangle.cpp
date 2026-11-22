#include "wrangle_shared.h"
#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;

int g_080de74c;
int g_080de750;
volatile char g_080de754;
int g_080de758;
volatile char g_080de75c;

bool wrangleMode1Worker(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool func_0806558c(Instr *i, int *row);
bool func_0807f2c4(Block *blk, Instr *mv);
bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe);
bool wrangleMode0Worker(workTable *wt, wrangleFunc f);
bool findInsertPositionMode0(workTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(workTable *wt);
bool func_0808158c(workTable *wt, wrangleCand *c);
int checkInsert(workTable *wt, Instr *in, InstructionPosition *pos, int *minPos);
bool buildCandidateList(workTable *wt);
bool func_080803dc(workTable *wt, wrangleCand *c);
bool DoInsert(workTable *wt, Instr *in, InstructionPosition *pos);
bool UndoInsert(workTable *wt, Instr *in);
int drag(workTable *wt, Instr *in, InstructionPosition *pos, int back, int depth);
int func_0807de00(workTable *wt, Instr *in, InstructionPosition *pos);
int func_0807e1f8(workTable *wt, Instr *in, InstructionPosition *pos);
int getMove(Block *blk, Instr *in, int now);
bool undoMovesTo(Block *blk, Instr *stop);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);
extern int errorOccurred;
extern bool quiet_080bbd98;

bool func_0807f360(workTable *wt, wrangleFunc f)
{
    g_080de288 = 0;
    if (wt->unk_09 != 2) {
        reportError("can't wrangle a non MID table\n");
        abortAfterError();
    }
    g_080de74c = 0;
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
    for (Instr *in = wt->pendingInstructions.head; in; in = in->unk_24.unk_00) {
        adjustMoveCount((Block *)wt, in, 1);
        if (!in->func_080558f4()) {
            reportError("failed to set loop sfx type\n");
            abortAfterError();
        }
    }
    if (wt->unk_38) {
        int row = wt->unk_30;
        if (row != 0 || wt->unk_34 != wt->unk_38) {
            row = false;
            reportError("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          wt->unk_30, wt->unk_34, wt->unk_38);
            return false;
        }
    } else {
        int row = wt->unk_30;
        if (row != 0 || wt->unk_34 >= 0) {
            row = false;
            reportError("wrangle - non loop set up correctly [%d,%d)\n", wt->unk_30, wt->unk_34);
            return false;
        }
    }
    if (!f) {
        reportError("Wrangle: passed a bad func\n");
        return false;
    }
    g_080de72c = 0;
    if (wt->unk_38)
        g_080de750 = wt->pendingInstructions.count / 40 + 3;
    else
        g_080de750 = wt->pendingInstructions.count / 5 + 1;
    if (g_080de750 < timeout_080b3010)
        g_080de750 = timeout_080b3010;
    if (!setWatchdog((char *)&g_080de754, g_080de750)) {
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
        if (g_080de754) {
            reportWarning("time out..\n");
            return false;
        }
        bool bFaulted = false;
        Instr *pending;
        int ret;
        Instr *m;
        int tmp;
        bool inserted = false;
        g_080de72c = 0;
        pending = 0;
        if (!buildCandidateList(wt)) {
            reportError("Wrangle: failed to build candidate list!\n");
            abortAfterError();
        }
        wtInstrList *done = &wt->completedInstructions;
        if (done->last())
            wt->unk_54.setFromLinearRow(done->last()->position.getRow());
        else
            wt->unk_54.setFromLinearRow(0);
        if (!wt->unk_4c) {
            Instr *todo = wt->pendingInstructions.head;
            if (!todo) {
                wt->unk_30 = 0;
                if (!wt->unk_38) {
                    int sz = wt->getCurrentDoneSize();
                    if (sz < 0) {
                        reportError("error getting current done size\n");
                        abortAfterError();
                        return false;
                    }
                    wt->unk_34 = sz;
                } else {
                    wt->unk_34 = wt->unk_38;
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
                        int lim2 = maxBranchLid * wt->unk_38;
                        int lim1 = (maxBranchLid - wt->unk_7a) * wt->unk_38;
                        g_080de72c = 0;
                        for (Instr *in = wt->completedInstructions.head; in; in = in->unk_24.unk_00) {
                            switch ((unsigned char)in->unk_138e) {
                            case 0:
                                break;
                            case 1:
                                if (in->position.unk_00 < lim1)
                                    func_080737b8(in);
                                break;
                            case 2:
                                if (in->position.unk_00 < lim2) {
                                    int saved = errorOccurred;
                                    errorOccurred = !quiet_080bbd98;
                                    in->leDump();
                                    errorOccurred = saved;
                                    func_080737b8(in);
                                }
                                break;
                            default:
                                reportError("bad case in sfx type\n");
                                abortAfterError();
                            }
                        }
                        if (!g_080de72c) {
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
                wt->unk_4c = 0;
                goto check_faulted;
            }
        } else {
            wrangleCand c;
            c.unk_00 = 0;
            c.unk_04 = -1;
            c.unk_06 = 0;
            wrangleCand mor;
            mor.unk_00 = 0;
            mor.unk_04 = -1;
            mor.unk_06 = 0;
            if (!f(wt, (int *)&c))
                goto faulted;
            tmp = wt->unk_38;
            bool isLoop = tmp != 0;
            int lsz = tmp;
            if (isLoop && (m = c.unk_00)->unk_142c) {
                int pipe = c.unk_06;
                if (m->unk_138c == 3) {
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
                    g_080de72c = wt->completedInstructions.head;
                    goto faulted;
                }
                if (!resolveMovePipe(wt, c.unk_00, pipe)) {
                    reportError("failed to set up a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!func_080803dc(wt, &c)) {
                c.unk_04 = c.unk_00->unk_141e;
                if (!func_080803dc(wt, &c))
                    goto faulted;
            }
            if (!c.unk_00)
                exitAfterCleanup(-1);
            pos.setFromLinearRow(c.unk_04);
            pos.setInstructionPipe(c.unk_06);
            int minPos = c.unk_04;
            g_080de72c = 0;
            if (!wt->lockLine(&pos)) {
                reportError("failed to lock a line\n");
                abortAfterError();
            }
            if (wt->unk_38) {
                Instr *in = c.unk_00;
                if (!in->unk_13d1)
                    drag(wt, in, &pos, wt->unk_38 >> 1, 0);
                else
                    drag(wt, in, &pos, wt->unk_38, 0);
            } else {
                func_0807de00(wt, c.unk_00, &pos);
                func_0807e1f8(wt, c.unk_00, &pos);
            }
            if (!wt->unlockLine(&pos)) {
                reportError("failed to unlock a line\n");
                abortAfterError();
            }
            g_080de72c = 0;
            ret = checkInsert(wt, c.unk_00, &pos, &minPos);
            Instr **ps = &pending;
            Instr **pc = &c.unk_00;
            int *pg = &g_080de74c;
            if (g_080de72c) {
                ps = &g_080de72c;
                pc = &g_080de72c;
                pg = &errorOccurred;
                goto faulted;
            }
            c.unk_00->func_08054008(false);
            int z1 = 1;
            int z2;
            if (ret & 0x100) {
                if (!wt->lockLine(&pos)) {
                    reportError("failed to lock line again\n");
                    abortAfterError();
                }
                int n = drag(wt, c.unk_00, &pos, wt->unk_38, 4);
                if (!wt->unlockLine(&pos)) {
                    reportError("failed to unlock line again\n");
                    abortAfterError();
                }
                if (n < 0) {
                    reportError("failed to drag setters\n");
                    abortAfterError();
                }
                if (n)
                    ret = checkInsert(wt, c.unk_00, &pos, &minPos);
                if ((ret & 0x100) && !pending) {
                    Instr *cand = c.unk_00;
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
            if (c.unk_00->unk_142c && !wt->unk_38) {
                if (!resolveMovePipe(wt, c.unk_00, c.unk_06)) {
                    reportError("failed to assign a move to a pipe\n");
                    abortAfterError();
                }
            }
            if (!DoInsert(wt, c.unk_00, &pos)) {
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
                if (!c.unk_00) {
                    reportError("no instruction set in mOR\n");
                    abortAfterError();
                }
                int r = getMove((Block *)wt, c.unk_00, c.unk_04);
                switch (r) {
                case 0:
                    if (g_080de72c) {
                        reportWarning("but fInst is now set after making a move.. - so clearing it\n");
                        g_080de72c = 0;
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
                        g_080de72c = wt->completedInstructions.head;
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
        wt->unk_4c = 0;
    check_faulted:
        if (bFaulted) {
            if (g_080de72c)
                g_080de72c->func_08054008(false);
            else
                g_080de72c = done->last();
            for (wtInstrList *tail = done;
                 (Instr *)*(VclPtrWord *)&tail->tail != g_080de72c;
                 tail = &wt->completedInstructions) {
                Instr *in = tail->last();
                in->clearTransientFlags();
                if (!UndoInsert(wt, in)) {
                    reportError("in finst track..undoinsert failed\n");
                    abortAfterError();
                    return false;
                }
                if (in->unk_1428 && !undoMovesTo((Block *)wt, in)) {
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
                in->unk_141c++;
                if (++in->unk_1418 <= in->unk_141a)
                    break;
                in->clearTransientFlags();
                if (in->unk_1428) {
                    if (!undoMovesTo((Block *)wt, in)) {
                        reportError("failed to undo move\n");
                        abortAfterError();
                    }
                    in = 0;
                }
                if (in) {
                    in->unk_1418 = 0;
                    in->unk_141a++;
                    if (wt->unk_38 && in->unk_141a > wt->unk_38 * 16) {
                        if (in->unk_1428) {
                            if (!undoMovesTo((Block *)wt, in)) {
                                reportError("failed to undo move\n");
                                abortAfterError();
                            }
                            in = 0;
                        }
                        if (in)
                            in->unk_141a = wt->unk_38 * 16;
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
    wt->unk_4c = 0;
    wt->unk_50 = 0;
    InstructionPosition pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    wt->dump();
    dynCalcStats(wt);
    g_080de74c++;
    for (; in; in = in->unk_24.unk_00) {
        short m = in->unk_13b2;
        if (m < in->unk_13b0)
            m = in->unk_13b0;
        int minRow = m;
        earliest = 0;
        if (!func_0806558c(in, &earliest)) {
            reportError("failed check cand\n");
            abortAfterError();
        }
        if (earliest < 0) {
            in->func_08054008(false);
            continue;
        }
        if (!in->hasExtraPredecessors && minRow < earliest)
            minRow = earliest;
        if (in->branchFlags) {
            if (g_080b336c) {
                minRow = g_080b336c - 2;
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
            RegPoolEntry *r = op->unk_18;
            if (!r)
                continue;
            for (int j = 0; j != r->partCount; j++) {
                if ((op->unk_07 >> j) & 1) {
                    if (isRegisterOperand(op, 1)) {
                        OpSlot *s = &op->unk_2c[j];
                        OperandLock *p = s->getParent();
                        if (!p) {
                            reportError("input had no link\n");
                            abortAfterError();
                            return false;
                        }
                        if (!p->unk_2c)
                            goto next;
                        if (s->unk_19 != 2) {
                            int t = p->unk_32 + p->unk_18 + s->unk_18;
                            if (t > minRow)
                                minRow = t;
                        }
                    }
                }
            }
            if (r->unk_20 && in != r->unk_20 && r->unk_20->position.unk_04 < 0)
                goto next;
        }
        if (wt->unk_38) {
            int type = (unsigned char)in->unk_138e;
            int lim = 0;
            switch (type) {
            case 0:
                break;
            case 1:
                if (wt->unk_84)
                    lim = (wt->unk_84->position.unk_03 - wt->unk_7a) * wt->unk_38;
                else
                    lim = wt->unk_80;
                break;
            case 2:
                if (wt->unk_84)
                    lim = wt->unk_84->position.unk_03 * wt->unk_38;
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
            in->unk_141e = row;
            if (!func_0807f2c4((Block *)wt, in)) {
                reportError("failed to add candidate\n");
                return false;
            }
        }
        if (!wt->unk_38 && wt->unk_50 > 16)
            break;
    next:;
    }
    return true;
}

bool func_080803dc(workTable *wt, wrangleCand *c)
{
    int pipe;
    int row = c->unk_04;
    Instr *in = c->unk_00;
    pipe = 0;
    if (in->branchFlags)
        row = in->unk_141e;
    if (in->unk_138c == 3)
        pipe = c->unk_06 & 1;
    else
        pipe = in->unk_138c != 1;
    if (findInsertPosition(wt, in, &row, &pipe)) {
        c->unk_04 = row;
        c->unk_06 = pipe;
        return true;
    }
    return false;
}



bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe)
{
    bool both = false;
    if (in->unk_138c == 3)
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
        *pipe = in->unk_138c != 1;
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
