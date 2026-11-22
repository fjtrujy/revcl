#include "os.h"
#include "genRegPool_shared.h"
#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void func_0804c220(char *p, int n);
RegPool *GetRP(unsigned t);
bool func_0806f610(Program *p);
bool func_0806f6a4(Program *p);
bool setRange(int start, int end, int lsz);
int func_0804bf50(unsigned int);
int setRangeFrom(workTable *t);
bool lockLiveRange(RegisterPartLockList *ll, OperandLock *lk, int *s, int *e);


extern const char assignLocalExternalError[];
extern const char assignLocalModeError[];
extern const char assignLocalCleanupError[];
extern const char assignLocalLocksError[];
extern const char assignLocalPendingError[];
extern const char assignLocalBadTableError[];
extern const char buildTTabNoBitsError[];
extern const char buildTTabOverlapError[];
extern const char buildTTabLiveRangeError[];
extern const char buildTTabLockListError[];
extern const char buildTTabNullError[];
extern const char initAllocMaxDevError[];
extern const char initAllocTooBigError[];
extern const char initAllocDoneTableError[];
extern const char initAllocNonZeroBaseError[];
extern const char setTInATabOverlapError[];
extern const char setTInATabAccessError[];
extern const char setInATabBuildError[];
extern const char setInATabCheckError[];
extern const char setInATabLocalIdError[];
extern const char setInATabVirtualError[];

bool assignLocalRegisters(Program *p, workTable *wt)
{
    int len = wt->getPhysicalLen();
    if (len < 0) {
        reportError(assignLocalBadTableError);
        exitAfterCleanup(-1);
    }
    if (wt->unk_94.count) {
        reportError(assignLocalPendingError, wt->unk_94.count);
        for (Instr *in = wt->unk_94.head; in; in = in->unk_24.unk_00)
            in->func_08054008(false);
        exitAfterCleanup(-1);
    }
    if (wt->unk_08 != 2) {
        reportError(assignLocalExternalError);
        exitAfterCleanup(-1);
    }
    unsigned int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        reportError(assignLocalModeError, mode);
        exitAfterCleanup(-1);
    }
    if (!wt->unk_0b) {
        reportError(assignLocalLocksError);
        exitAfterCleanup(-1);
    }
    if (len) {
        setRange(wt->func_0807411c(), wt->func_080741c0(), wt->func_080741b4());
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->func_08088e54(wt))
                return false;
        }
        if (!func_0806f6a4(p)) {
            reportError(assignLocalCleanupError);
            abortAfterError();
            return false;
        }
    }
    wt->unk_08 = 3;
    return true;
}

void clearATab()
{
    func_0804c220(aTab_080bce40, 0x20000);
}

void clearTTab()
{
    func_0804c220(tTab_080dce40, 0x1000);
}

void genRegPool::func_08089a4c()
{
    func_0804c220(unk_30, 0x20);
}

bool genRegPool::build_tTab(RegPoolEntry *reg, unsigned char mask)
{
    func_0804c220(tTab_080dce40, 0x1000);
    if (!mask)
        return true;
    char set = 0;
    if (!reg) {
        reportError(buildTTabNullError);
        return false;
    }
    int lsz = g_080b336c;
    for (int i = 0; i != reg->partCount; i++) {
        bool on = (mask >> i) & 1;
        if (!on)
            continue;
        RegisterPartLockList *ll = (RegisterPartLockList *)reg->func_0805a908(i);
        if (!ll) {
            reportError(buildTTabLockListError, reg->registerName());
            return false;
        }
        OperandLock *lk = ll->unk_1c;
        if (!lk)
            continue;
        bool shared = ll->unk_2b != 0;
        set |= 1 << i;
        if (lsz && ll->unk_2a)
            shared = true;
        for (; lk; lk = lk->unk_08) {
            int s = 0, e = 0;
            if (!lockLiveRange(ll, lk, &s, &e)) {
                reportError(buildTTabLiveRangeError);
                exitAfterCleanup(-1);
            }
            if (s < 0) {
                if (lk->unk_2d != 2) {
                    reportError("not extern in .. -ve s\n");
                    return false;
                }
                if (lsz) {
                    if (e == 30000)
                        s = 0;
                    else
                        s = (e / lsz) * lsz;
                } else
                    s = 0;
            } else {
                if (lk->unk_2d == 2) {
                    reportError("extern in .. rl_s>0\n");
                    return false;
                }
            }
            if (e == 30000) {
                if (lsz)
                    e = s + lsz - s % lsz;
                else
                    e = tEnd_080bce2c;
            }
            int r = lk->unk_30;
            if (r >= 0)
                tTab_080dce40[r] |= 0x80;
            for (int c = s; c < e; c++) {
                int idx = c;
                if (lsz)
                    idx = c % lsz;
                unsigned char v = tTab_080dce40[idx];
                if (((v >> i) & 1) && !shared) {
                    int overlap = ll->unk_2b != 0;
                    reportError(buildTTabOverlapError, overlap);
                    ll->checkCounts();
                    dumpTTab();
                    return false;
                }
                tTab_080dce40[idx] = v | (1 << i);
            }
        }
    }
    if (!set) {
        reportError(buildTTabNoBitsError, reg->registerName());
        exitAfterCleanup(-1);
    }
    return true;
}

bool genRegPool::initAlloc(int start, int end)
{
    func_0804c220(aTab_080bce40, 0x20000);
    func_0804c220(unk_30, 0x20);
    if (start) {
        reportError(initAllocNonZeroBaseError);
        return false;
    }
    if (end < start) {
        reportError(initAllocDoneTableError);
        return false;
    }
    if (g_080b336c && end != g_080b336c)
        reportError("loop mismatch\n");
    tStart_080bce28 = start;
    tEnd_080bce2c = end;
    if (end > 0xfff) {
        reportError(initAllocTooBigError);
        return false;
    }
    RegPoolEntry *e = registerEntries;
    if (!e) {
        reportError("RegPool:Dump - no table?\n");
        return false;
    }
    if (entryCount > 0x20) {
        reportError(initAllocMaxDevError);
        return false;
    }
    nRegs_080bce24 = entryCount;
    maxParts_080bce20 = 0;
    int i = 0;
    int count = entryCount;
    for (; i != count; i++, e++) {
        if (e->unk_16)
            unk_30[i] = (1 << e->partCount) - 1;
        if (e->partCount > maxParts_080bce20) {
            maxParts_080bce20 = e->partCount;
            count = entryCount;
        }
    }
    return true;
}

void genRegPool::dumpTTab()
{
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++) {
        char c = tTab_080dce40[i];
        if (c)
            ;
    }
}

int genRegPool::countTTab()
{
    int n = 0;
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++)
        if (tTab_080dce40[i])
            n++;
    return n;
}

int genRegPool::fitCost(int reg)
{
    int total = 0;
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++) {
        unsigned char t = tTab_080dce40[i];
        int m = t & 0x7f;
        if (((unsigned char)unk_30[reg] & m) != m)
            return -1;
        char *row = &aTab_080bce40[i * 32];
        unsigned char a = row[reg];
        if (t & a)
            return -1;
        int n = func_0804bf50((t | a) & 0x7f);
        if (n > maxParts_080bce20) {
            reportError("nbits overrun \n");
            exitAfterCleanup(-1);
        }
        total += maxParts_080bce20 - n;
    }
    if (total < 0)
        total = 0;
    return total;
}

bool genRegPool::set_t_in_aTab(int reg)
{
    int end = tEnd_080bce2c;
    for (int i = tStart_080bce28; i < end; i++) {
        unsigned char c = tTab_080dce40[i];
        int t = c;
        if (((unsigned char)unk_30[reg] & t & 0x7f) != (c & 0x7f)) {
            reportError(setTInATabAccessError, reg);
            return false;
        }
        char *row = &aTab_080bce40[i * 32];
        unsigned char a = row[reg];
        if (a & t) {
            reportError(setTInATabOverlapError,
                          i, reg, a, t);
            return false;
        }
        row[reg] = a | c;
    }
    return true;
}

void genRegPool::dumpATab()
{
    for (int i = tStart_080bce28; i != tEnd_080bce2c; i++)
        for (int r = 0; r != nRegs_080bce24; r++) {
            for (int p = 0; p != maxParts_080bce20; p++) {
                int bit = (unsigned char)unk_30[r] & (1 << p);
                if (bit != 0)
                    bit = 1;
            }
        }
}

int genRegPool::bestReg()
{
    int best = 100000000;
    int which = -1;
    for (int r = 0; r != nRegs_080bce24; r++) {
        int c = fitCost(r);
        if (c < 0)
            continue;
        if (c == 0)
            return r;
        if (c < best) {
            best = c;
            which = r;
        }
    }
    return which;
}

bool genRegPool::set_in_aTab(regUse *ru)
{
    RegPoolEntry *reg = ru->unk_08;
    if (!reg) {
        reportError("regref had no register\n");
        return false;
    }
    if (ownsRegisterClass(reg)) {
        if (reg->isVirtual) {
            reportError(setInATabVirtualError,
                          reg->registerName());
            exitAfterCleanup(-1);
        }
        int id = reg->regIndex();
        if (id < 0) {
            reportError(setInATabLocalIdError);
            exitAfterCleanup(-1);
        }
        if (reg->unk_16) {
            unsigned char mask = ru->unk_0e | ru->unk_0f | ru->unk_10;
            if (!mask) {
                reportError("error here\n");
                exitAfterCleanup(-1);
            }
            int set = 0;
            for (int i = 0; i != reg->partCount; i++) {
                bool on = (mask >> i) & 1;
                if (on && !reg->func_0805a908(i)) {
                    reportError("failed to get rll\n");
                    return false;
                }
                RegisterPartLockList *ll = (RegisterPartLockList *)reg->func_0805a908(i);
                if (ll) {
                    if (!ll->unk_03)
                        set |= 1 << i;
                    ll->unk_03 = 1;
                }
            }
            if (set) {
                if (!build_tTab(reg, set)) {
                    reportError(setInATabBuildError,
                                  reg->registerName(), reg->registerName());
                    return false;
                }
                if (!set_t_in_aTab(id)) {
                    reportError(setInATabCheckError,
                                  reg->registerName(), id);
                    dumpTTab();
                    dumpATab();
                    return false;
                }
            }
        }
    }
    return true;
}
