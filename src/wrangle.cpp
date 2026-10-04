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
char g_080de75c;

bool func_0807f560(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool func_0806558c(Instr *i, int *row);
bool func_0807f2c4(Block *blk, Instr *mv);
bool func_08080460(workTable *wt, Instr *in, int *row, int *pipe);
bool func_080807e8(workTable *wt, wrangleFunc f);
bool func_08081610(workTable *wt, Instr *in, int *row, int *pipe);
bool buildCandidateList2(workTable *wt);
bool func_0808158c(workTable *wt, wrangleCand *c);
int checkInsert(workTable *wt, Instr *in, cls08058718 *pos, int *minPos);
bool buildCandidateList(workTable *wt);
bool func_080803dc(workTable *wt, wrangleCand *c);
bool DoInsert(workTable *wt, Instr *in, cls08058718 *pos);
bool UndoInsert(workTable *wt, Instr *in);
int drag(workTable *wt, Instr *in, cls08058718 *pos, int back, int depth);
int func_0807de00(workTable *wt, Instr *in, cls08058718 *pos);
int func_0807e1f8(workTable *wt, Instr *in, cls08058718 *pos);
int getMove(Block *blk, Instr *in, int now);
bool undoMovesTo(Block *blk, Instr *stop);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);
extern int errorOccurred;
extern bool quiet_080bbd98;

static inline void dumpInstrQuiet(Instr *in)
{
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    in->leDump();
    errorOccurred = saved;
}

bool func_0807f360(workTable *wt, wrangleFunc f)
{
    g_080de288 = 0;
    if (wt->unk_09 != 2) {
        func_0804ad38("can't wrangle a non MID table\n");
        func_0804aeb0();
    }
    g_080de74c = 0;
    wt->unk_ad = 0;
    wt->unk_ac = 0;
    g_080de284 = 1;
    wt->unk_84 = 0;
    wt->unk_7c = -1;
    wt->unk_80 = 0;
    wt->unk_82 = 0;
    wt->unk_b0 = 0;
    wt->unk_b4 = 0;
    wt->unk_b8 = 0;
    for (Instr *in = wt->unk_94.unk_00; in; in = in->unk_24.unk_00) {
        adjustMoveCount((Block *)wt, in, 1);
        if (!in->func_080558f4()) {
            func_0804ad38("failed to set loop sfx type\n");
            func_0804aeb0();
        }
    }
    if (wt->unk_38) {
        if (wt->unk_30 != 0 || wt->unk_34 != wt->unk_38) {
            func_0804ad38("wrangle : loop not set up in correctly [%d,%d) vs %d\n",
                          wt->unk_30, wt->unk_34, wt->unk_38);
            return false;
        }
    } else {
        if (wt->unk_30 != 0 || wt->unk_34 >= 0) {
            func_0804ad38("wrangle - non loop set up correctly [%d,%d)\n", wt->unk_30, wt->unk_34);
            return false;
        }
    }
    if (!f) {
        func_0804ad38("Wrangle: passed a bad func\n");
        return false;
    }
    g_080de72c = 0;
    if (wt->unk_38)
        g_080de750 = wt->unk_94.unk_08 / 40 + 3;
    else
        g_080de750 = wt->unk_94.unk_08 / 5 + 1;
    if (g_080de750 < timeout_080b3010)
        g_080de750 = timeout_080b3010;
    if (!setWatchdog((char *)&g_080de754, g_080de750)) {
        func_0804ad38("failed to set OS_setWatchdog\n");
        return false;
    }
    bool r = func_0807f560(wt, f);
    if (!clearWatchdog()) {
        func_0804ad38("failed to clear watchdog timer\n");
        return false;
    }
    return r;
}

bool func_0807f560(workTable *wt, wrangleFunc f)
{
    cls08058718 pos;
    for (;;) {
        if (g_080de754) {
            func_0804ac7c("time out..\n");
            return false;
        }
        bool bFaulted = false;
        bool inserted = false;
        g_080de72c = 0;
        Instr *pending = 0;
        if (!buildCandidateList(wt)) {
            func_0804ad38("Wrangle: failed to build candidate list!\n");
            func_0804aeb0();
        }
        wtInstrList *done = &wt->unk_a0;
        if (done->unk_04)
            wt->unk_54.func_0805874c(done->unk_04->unk_13e8.unk_00);
        else
            wt->unk_54.func_0805874c(0);
        if (!wt->unk_4c) {
            if (wt->unk_94.unk_00) {
                wt->dump();
                bFaulted = true;
                wt->unk_4c = 0;
                goto check_faulted;
            }
            wt->unk_30 = 0;
            if (!wt->unk_38) {
                int sz = wt->getCurrentDoneSize();
                if (sz < 0) {
                    func_0804ad38("error getting current done size\n");
                    func_0804aeb0();
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
                    func_0804ad38("failed to lid stats ?\n");
                    return false;
                }
                bool b2 = minSfx2 >= 0 && minSfx2 < maxBranchLid;
                bool b1 = minSfx1 >= 0 && minSfx1 < maxBranchLid - wt->unk_7a;
                if (b2 || b1) {
                    wt->unk_7c = maxBranchLid;
                    int lim2 = maxBranchLid * wt->unk_38;
                    int lim1 = (maxBranchLid - wt->unk_7a) * wt->unk_38;
                    g_080de72c = 0;
                    for (Instr *in = wt->unk_a0.unk_00; in; in = in->unk_24.unk_00) {
                        switch ((unsigned char)in->unk_138e) {
                        case 0:
                            break;
                        case 1:
                            if (in->unk_13e8.unk_00 < lim1)
                                func_080737b8(in);
                            break;
                        case 2:
                            if (in->unk_13e8.unk_00 < lim2) {
                                dumpInstrQuiet(in);
                                func_080737b8(in);
                            }
                            break;
                        default:
                            func_0804ad38("bad case in sfx type\n");
                            func_0804aeb0();
                        }
                    }
                    if (!g_080de72c) {
                        func_0804ad38("no fInst set after unwinding from bad side effects\n");
                        func_0804aeb0();
                    }
                    goto faulted;
                }
            }
            if (!wt->changeMode(3)) {
                func_0804ad38("failed to change code mode to raw\n");
                func_0804aeb0();
            }
            return true;
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
            int lsz = wt->unk_38;
            if (lsz && c.unk_00->unk_142c) {
                int pipe = c.unk_06;
                if (c.unk_00->unk_138c == 3) {
                    if (wt->unk_b4 == lsz)
                        pipe = 1;
                    else if (wt->unk_b8 == lsz)
                        pipe = 0;
                    else if (wt->unk_ac)
                        pipe = 0;
                    else
                        pipe = wt->unk_b4 > wt->unk_b8;
                }
                if (pipe == 1 && wt->unk_b8 == lsz && !wt->unk_ac) {
                    wt->unk_ac = 1;
                    wt->unk_ad = 0;
                    g_080de72c = wt->unk_a0.unk_00;
                    goto faulted;
                }
                if (!resolveMovePipe(wt, c.unk_00, pipe)) {
                    func_0804ad38("failed to set up a move to a pipe\n");
                    func_0804aeb0();
                }
            }
            if (!func_080803dc(wt, &c)) {
                c.unk_04 = c.unk_00->unk_141e;
                if (!func_080803dc(wt, &c))
                    goto faulted;
            }
            if (!c.unk_00)
                func_0804adf0(-1);
            pos.func_0805874c(c.unk_04);
            pos.func_0805878c(c.unk_06);
            int minPos = c.unk_04;
            g_080de72c = 0;
            if (!wt->lockLine(&pos)) {
                func_0804ad38("failed to lock a line\n");
                func_0804aeb0();
            }
            if (wt->unk_38) {
                if (c.unk_00->unk_13d1)
                    drag(wt, c.unk_00, &pos, wt->unk_38, 0);
                else
                    drag(wt, c.unk_00, &pos, wt->unk_38 >> 1, 0);
            } else {
                func_0807de00(wt, c.unk_00, &pos);
                func_0807e1f8(wt, c.unk_00, &pos);
            }
            if (!wt->unlockLine(&pos)) {
                func_0804ad38("failed to unlock a line\n");
                func_0804aeb0();
            }
            g_080de72c = 0;
            int ret = checkInsert(wt, c.unk_00, &pos, &minPos);
            if (g_080de72c)
                goto faulted;
            c.unk_00->func_08054008(false);
            if (ret & 0x100) {
                if (!wt->lockLine(&pos)) {
                    func_0804ad38("failed to lock line again\n");
                    func_0804aeb0();
                }
                int n = drag(wt, c.unk_00, &pos, wt->unk_38, 4);
                if (!wt->unlockLine(&pos)) {
                    func_0804ad38("failed to unlock line again\n");
                    func_0804aeb0();
                }
                if (n < 0) {
                    func_0804ad38("failed to drag setters\n");
                    func_0804aeb0();
                }
                if (n)
                    ret = checkInsert(wt, c.unk_00, &pos, &minPos);
                if ((ret & 0x100) && !pending) {
                    pending = c.unk_00;
                    mor = c;
                    goto check_pending;
                }
            }
            if (ret)
                goto faulted;
            if (c.unk_00->unk_142c && !wt->unk_38) {
                if (!resolveMovePipe(wt, c.unk_00, c.unk_06)) {
                    func_0804ad38("failed to assign a move to a pipe\n");
                    func_0804aeb0();
                }
            }
            if (!DoInsert(wt, c.unk_00, &pos)) {
                func_0804ad38("was ok to insert..but failed!\n");
                wt->dump();
                func_0804aeb0();
            } else
                inserted = true;
            if (!inserted) {
                func_0804ad38("got here - but inserted not set bFaulted=%d ret=%x\n", bFaulted, ret);
                func_0804aeb0();
            }
        check_pending:
            if (pending) {
                if (bFaulted) {
                    func_0804ad38("bFaulted is set with a pending move candidate\n");
                    func_0804aeb0();
                }
                c = mor;
                if (!c.unk_00) {
                    func_0804ad38("no instruction set in mOR\n");
                    func_0804aeb0();
                }
                int r = getMove((Block *)wt, c.unk_00, c.unk_04);
                switch (r) {
                case 0:
                    if (g_080de72c) {
                        func_0804ac7c("but fInst is now set after making a move.. - so clearing it\n");
                        g_080de72c = 0;
                    }
                    break;
                case 1:
                    if (wt->unk_ad && !wt->unk_ac) {
                        wt->unk_ac = 1;
                        wt->unk_ad = 0;
                        g_080de72c = wt->unk_a0.unk_00;
                    }
                    break;
                case -1:
                case 2:
                case 3:
                    break;
                case 4:
                    func_0804ad38("a move isn't needed after all\n");
                    func_0804aeb0();
                    break;
                default:
                    func_0804ad38("oh no genmove failed .. return code %d!\n", r);
                    wt->dump();
                    func_0804aeb0();
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
                g_080de72c = done->unk_04;
            while (done->unk_04 != g_080de72c) {
                Instr *in = done->unk_04;
                in->func_08054214();
                if (!UndoInsert(wt, in)) {
                    func_0804ad38("in finst track..undoinsert failed\n");
                    func_0804aeb0();
                    return false;
                }
                if (in->unk_1428 && !undoMovesTo((Block *)wt, in)) {
                    func_0804ad38("failed to undo move\n");
                    func_0804aeb0();
                }
            }
            while (wt->unk_a0.unk_00) {
                Instr *in = done->unk_04;
                if (!UndoInsert(wt, in)) {
                    func_0804ad38("undo insert: failed to remove done list tail\n");
                    func_0804aeb0();
                    return false;
                }
                in->unk_141c++;
                if (++in->unk_1418 <= in->unk_141a)
                    break;
                in->func_08054214();
                if (in->unk_1428) {
                    if (!undoMovesTo((Block *)wt, in)) {
                        func_0804ad38("failed to undo move\n");
                        func_0804aeb0();
                    }
                    in = 0;
                }
                if (in) {
                    in->unk_1418 = 0;
                    in->unk_141a++;
                    if (wt->unk_38 && in->unk_141a > wt->unk_38 * 16) {
                        if (in->unk_1428) {
                            if (!undoMovesTo((Block *)wt, in)) {
                                func_0804ad38("failed to undo move\n");
                                func_0804aeb0();
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
    in = wt->unk_94.unk_00;
    wt->unk_4c = 0;
    wt->unk_50 = 0;
    cls08058718 pos;
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
            func_0804ad38("failed check cand\n");
            func_0804aeb0();
        }
        if (earliest < 0) {
            in->func_08054008(false);
            continue;
        }
        if (!in->unk_1410 && minRow < earliest)
            minRow = earliest;
        if (in->unk_1388) {
            if (g_080b336c) {
                minRow = g_080b336c - 2;
                if (minRow < 0) {
                    func_0804ad38("branch at bad pos for loop\n");
                    func_0804aeb0();
                }
            } else {
                if (wt->unk_94.unk_08 > 1)
                    continue;
                int d = wt->getCurrentDoneSize();
                if (d < 0) {
                    func_0804ad38("failed to get current done size\n");
                    return false;
                }
                d -= 2;
                if (minRow < d)
                    minRow = d;
                if (minRow < 0)
                    minRow = 0;
            }
        }
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r = op->unk_18;
            if (!r)
                continue;
            for (int j = 0; j != r->unk_28; j++) {
                if (!((op->unk_07 >> j) & 1))
                    continue;
                if (!op->isInReg())
                    continue;
                OpSlot *s = &op->unk_2c[j];
                cls0805bcc4 *p = s->getParent();
                if (!p) {
                    func_0804ad38("input had no link\n");
                    func_0804aeb0();
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
            Instr *mv = r->unk_20;
            if (mv && in != mv && mv->unk_13e8.unk_04 < 0)
                goto next;
        }
        if (wt->unk_38) {
            int lim = 0;
            switch ((unsigned char)in->unk_138e) {
            case 0:
                break;
            case 1:
                if (wt->unk_84)
                    lim = (wt->unk_84->unk_13e8.unk_03 - wt->unk_7a) * wt->unk_38;
                else
                    lim = wt->unk_80;
                break;
            case 2:
                if (wt->unk_84)
                    lim = wt->unk_84->unk_13e8.unk_03 * wt->unk_38;
                else
                    lim = wt->unk_82;
                break;
            }
            if (minRow < lim)
                minRow = lim;
        }
        row = minRow;
        pipe = 0;
        if (func_08080460(wt, in, &row, &pipe)) {
            in->unk_141e = row;
            if (!func_0807f2c4((Block *)wt, in)) {
                func_0804ad38("failed to add candidate\n");
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
    if (in->unk_1388)
        row = in->unk_141e;
    if (in->unk_138c == 3)
        pipe = c->unk_06 & 1;
    else
        pipe = in->unk_138c != 1;
    if (func_08080460(wt, in, &row, &pipe)) {
        c->unk_04 = row;
        c->unk_06 = pipe;
        return true;
    }
    return false;
}

bool func_08080460(workTable *wt, Instr *in, int *row, int *pipe)
{
    bool both = false;
    if (in->unk_138c == 3)
        both = true;
    {
        cls08058718 pos;
        if (both) {
            int p;
            int best;
            int r;
            for (;;) {
                best = 30000;
                p = *pipe;
                for (; p <= 1; p++) {
                    r = *row;
                    pos.func_0805878c(p);
                    pos.func_0805874c(r);
                    if (!(checkInsert(wt, in, &pos, &r) & ~0x100)) {
                        *row = r;
                        *pipe = p;
                        return true;
                    }
                    if (r < best)
                        best = r;
                }
                if (best >= 30000)
                    return false;
                if (best == *row) {
                    func_0804ad38("min_nl was nl and all failed .. %d->%d\n", best, best);
                    func_0804aeb0();
                }
                *row = best;
                *pipe = 0;
            }
        } else {
            *pipe = in->unk_138c != 1;
            pos.func_0805874c(*row);
            pos.func_0805878c(*pipe);
            while (checkInsert(wt, in, &pos, row) & ~0x100) {
                if (*row >= 30000)
                    return false;
                pos.func_0805874c(*row);
            }
            if (*row >= 30000)
                return false;
        }
    }
    return true;
}
