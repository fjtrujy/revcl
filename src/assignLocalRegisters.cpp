#include "genRegPool_shared.h"

extern const char assignLocalExternalError[];
extern const char assignLocalModeError[];
extern const char assignLocalCleanupError[];
extern const char assignLocalLocksError[];
extern const char assignLocalPendingError[];
extern const char assignLocalBadTableError[];

bool assignLocalRegisters(Program *p, workTable *wt)
{
    int len = wt->getPhysicalLen();
    if (len < 0) {
        func_0804ad38(assignLocalBadTableError);
        func_0804adf0(-1);
    }
    if (wt->unk_94.unk_08) {
        func_0804ad38(assignLocalPendingError, wt->unk_94.unk_08);
        for (Instr *in = wt->unk_94.unk_00; in; in = in->unk_24.unk_00)
            in->func_08054008(false);
        func_0804adf0(-1);
    }
    if (wt->unk_08 != 2) {
        func_0804ad38(assignLocalExternalError);
        func_0804adf0(-1);
    }
    unsigned int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        func_0804ad38(assignLocalModeError, mode);
        func_0804adf0(-1);
    }
    if (!wt->unk_0b) {
        func_0804ad38(assignLocalLocksError);
        func_0804adf0(-1);
    }
    if (len) {
        setRange(wt->func_0807411c(), wt->func_080741c0(), wt->func_080741b4());
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->func_08088e54(wt))
                return false;
        }
        if (!func_0806f6a4(p)) {
            func_0804ad38(assignLocalCleanupError);
            func_0804aeb0();
            return false;
        }
    }
    wt->unk_08 = 3;
    return true;
}

