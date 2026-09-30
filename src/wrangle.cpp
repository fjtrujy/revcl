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

bool func_0807f560(workTable *wt, wrangleFunc f);

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
