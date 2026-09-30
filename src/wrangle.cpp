#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;

int g_080de74c;
int g_080de750;
char g_080de754;
int g_080de758;
char g_080de75c;

bool func_0807f560(workTable *wt, wrangleFunc f);
bool dynCalcStats(workTable *wt);
bool func_0806558c(Instr *i, int *row);
bool func_0807f2c4(Block *blk, Instr *mv);
bool func_08080460(workTable *wt, Instr *in, int *row, int *pipe);
bool func_080807e8(workTable *wt, wrangleFunc f);
bool func_08081610(workTable *wt, Instr *in, int *row, int *pipe);
int checkInsert(workTable *wt, Instr *in, cls08058718 *pos, int *minPos);

struct wrangleCand {
    Instr *unk_00;
    short unk_04;       // row
    char unk_06;        // pipe
};

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
    if (!setWatchdog(&g_080de754, g_080de750)) {
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
                if ((op->unk_07 >> j) & 1) {
                    if (op->isInReg()) {
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
        return true;
    }
}

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
        g_080de758 = wt->unk_94.unk_08 / 30 + 6;
    else
        g_080de758 = wt->unk_94.unk_08 / 5 + 4;
    if (g_080de758 < timeout_080b3010)
        g_080de758 = timeout_080b3010;
    if (!setWatchdog(&g_080de75c, g_080de758)) {
        func_0804ad38("failed to set OS_setWatchdog\n");
        return false;
    }
    bool r = func_080807e8(wt, f);
    if (!clearWatchdog()) {
        func_0804ad38("failed to clear watchdog timer\n");
        return false;
    }
    return r;
}

bool buildCandidateList2(workTable *wt)
{
    Instr *in;
    in = wt->unk_94.unk_00;
    wt->unk_4c = 0;
    wt->unk_50 = 0;
    cls08058718 pos;
    int earliest;
    int pipe = 0;
    int row = 0;
    dynCalcStats(wt);
    for (; in; in = in->unk_24.unk_00) {
        short m = in->unk_13b2;
        if (m < in->unk_13b0)
            m = in->unk_13b0;
        int minRow = m;
        earliest = 0;
        if (!func_0806558c(in, &earliest)) {
            func_0804ad38("failed check cand\n");
            func_0804adf0(-1);
        }
        if (earliest < 0) {
            in->func_08054008(false);
            continue;
        }
        if (!in->unk_1410 && minRow <= earliest)
            minRow = earliest;
        if (in->unk_1388) {
            if (g_080b336c) {
                minRow = g_080b336c - 2;
                if (minRow < 0) {
                    func_0804ad38("branch at bad pos for loop\n");
                    func_0804adf0(-1);
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
                if ((op->unk_07 >> j) & 1) {
                    if (op->isInReg()) {
                        OpSlot *s = &op->unk_2c[j];
                        cls0805bcc4 *p = s->getParent();
                        if (!p) {
                            func_0804ad38("input had no link\n");
                            func_0804adf0(-1);
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
            if (in->unk_13f0.unk_00 && minRow < in->unk_140c - 2 * wt->unk_38)
                minRow = in->unk_140c - 2 * wt->unk_38;
        }
        in->func_08054008(false);
        row = minRow;
        pipe = 0;
        if (func_08081610(wt, in, &row, &pipe)) {
            in->unk_141e = row;
            if (!func_0807f2c4((Block *)wt, in)) {
                func_0804ad38("failed to add candidate\n");
                return false;
            }
        }
        if (!wt->unk_38 && wt->unk_50 > 4)
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
    if (in->unk_1388)
        row = in->unk_141e;
    if (in->unk_138c == 3)
        pipe = c->unk_06 & 1;
    else
        pipe = in->unk_138c != 1;
    if (func_08081610(wt, in, &row, &pipe)) {
        c->unk_04 = row;
        c->unk_06 = pipe;
        return true;
    }
    return false;
}

bool func_08081610(workTable *wt, Instr *in, int *row, int *pipe)
{
    bool both = false;
    if (in->unk_138c == 3)
        both = true;
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
                func_0804adf0(-1);
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
        return true;
    }
}

bool func_080817d4(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->unk_4c; in; in = in->unk_2c) {
        if (in->unk_13e4 < min) {
            best = in;
            min = in->unk_13e4;
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = best->unk_141e + best->unk_141c;
    if (!best->unk_1428 && c->unk_04 < wt->unk_54.getRow())
        c->unk_04 = wt->unk_54.getRow();
    c->unk_06 = best->unk_1418 & 1;
    return true;
}

bool func_0808184c(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->unk_4c; in; in = in->unk_2c) {
        if (in->unk_13e4 < min) {
            best = in;
            min = in->unk_13e4;
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = best->unk_141e + best->unk_141c;
    if (!best->unk_1428 && c->unk_04 < wt->unk_54.getRow() - 64)
        c->unk_04 = wt->unk_54.getRow() - 64;
    if (c->unk_04 < 0)
        c->unk_04 = 0;
    c->unk_06 = best->unk_1418 & 1;
    return true;
}
