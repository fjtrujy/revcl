#include "os.h"
#include "genRegPool_shared.h"
#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *GetRP(unsigned t);
bool resetProgramAllocationsForFirstPass(Program *p);
bool resetProgramAllocationsForSecondPass(Program *p);
bool setRange(int start, int end, int lsz);
int countSetBits(unsigned int);
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
    if (wt->pendingInstructions.count) {
        reportError(assignLocalPendingError, wt->pendingInstructions.count);
        for (Instr *in = wt->pendingInstructions.head; in; in = in->pendingLink.nextInstruction)
            in->dumpOperandDiagnostics(false);
        exitAfterCleanup(-1);
    }
    if (wt->allocationMode != 2) {
        reportError(assignLocalExternalError);
        exitAfterCleanup(-1);
    }
    unsigned int mode = wt->codeModeValue;
    if ((unsigned char)mode != 3) {
        reportError(assignLocalModeError, mode);
        exitAfterCleanup(-1);
    }
    if (!wt->unk_0b) {
        reportError(assignLocalLocksError);
        exitAfterCleanup(-1);
    }
    if (len) {
        setRange(wt->getRangeStart(), wt->getRangeEnd(), wt->getLoopSize());
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->assignRegistersForWorkTable(wt))
                return false;
        }
        if (!resetProgramAllocationsForSecondPass(p)) {
            reportError(assignLocalCleanupError);
            abortAfterError();
            return false;
        }
    }
    wt->allocationMode = 3;
    return true;
}

void clearATab()
{
    clearRawBytes(allocationRegisterTimeline, 0x20000);
}

void clearTTab()
{
    clearRawBytes(candidateRegisterTimeline, 0x1000);
}

void genRegPool::clearRegisterAllocationMask()
{
    clearRawBytes(unk_30, 0x20);
}

bool genRegPool::build_tTab(RegPoolEntry *reg, unsigned char mask)
{
    clearRawBytes(candidateRegisterTimeline, 0x1000);
    if (!mask)
        return true;
    char set = 0;
    if (!reg) {
        reportError(buildTTabNullError);
        return false;
    }
    int lsz = activeLoopSize;
    for (int i = 0; i != reg->partCount; i++) {
        bool on = (mask >> i) & 1;
        if (!on)
            continue;
        RegisterPartLockList *ll = (RegisterPartLockList *)reg->getPartPayload(i);
        if (!ll) {
            reportError(buildTTabLockListError, reg->registerName());
            return false;
        }
        OperandLock *lk = ll->outputLocks;
        if (!lk)
            continue;
        bool shared = ll->unk_2b != 0;
        set |= 1 << i;
        if (lsz && ll->externalOutputCount)
            shared = true;
        for (; lk; lk = lk->nextSibling) {
            int s = 0, e = 0;
            if (!lockLiveRange(ll, lk, &s, &e)) {
                reportError(buildTTabLiveRangeError);
                exitAfterCleanup(-1);
            }
            if (s < 0) {
                if (lk->ownerType != 2) {
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
                if (lk->ownerType == 2) {
                    reportError("extern in .. rl_s>0\n");
                    return false;
                }
            }
            if (e == 30000) {
                if (lsz)
                    e = s + lsz - s % lsz;
                else
                    e = allocationTimelineEnd;
            }
            int r = lk->loopRow;
            if (r >= 0)
                candidateRegisterTimeline[r] |= 0x80;
            for (int c = s; c < e; c++) {
                int idx = c;
                if (lsz)
                    idx = c % lsz;
                unsigned char v = candidateRegisterTimeline[idx];
                if (((v >> i) & 1) && !shared) {
                    int overlap = ll->unk_2b != 0;
                    reportError(buildTTabOverlapError, overlap);
                    ll->checkCounts();
                    dumpTTab();
                    return false;
                }
                candidateRegisterTimeline[idx] = v | (1 << i);
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
    clearRawBytes(allocationRegisterTimeline, 0x20000);
    clearRawBytes(unk_30, 0x20);
    if (start) {
        reportError(initAllocNonZeroBaseError);
        return false;
    }
    if (end < start) {
        reportError(initAllocDoneTableError);
        return false;
    }
    if (activeLoopSize && end != activeLoopSize)
        reportError("loop mismatch\n");
    allocationTimelineStart = start;
    allocationTimelineEnd = end;
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
    allocationRegisterCount = entryCount;
    maximumAllocationParts = 0;
    int i = 0;
    int count = entryCount;
    for (; i != count; i++, e++) {
        if (e->enabledPhysicalRegister)
            unk_30[i] = (1 << e->partCount) - 1;
        if (e->partCount > maximumAllocationParts) {
            maximumAllocationParts = e->partCount;
            count = entryCount;
        }
    }
    return true;
}

void genRegPool::dumpTTab()
{
    for (int i = allocationTimelineStart; i < allocationTimelineEnd; i++) {
        char c = candidateRegisterTimeline[i];
        if (c)
            ;
    }
}

int genRegPool::countTTab()
{
    int n = 0;
    for (int i = allocationTimelineStart; i < allocationTimelineEnd; i++)
        if (candidateRegisterTimeline[i])
            n++;
    return n;
}

int genRegPool::fitCost(int reg)
{
    int total = 0;
    for (int i = allocationTimelineStart; i < allocationTimelineEnd; i++) {
        unsigned char t = candidateRegisterTimeline[i];
        int m = t & 0x7f;
        if (((unsigned char)unk_30[reg] & m) != m)
            return -1;
        char *row = &allocationRegisterTimeline[i * 32];
        unsigned char a = row[reg];
        if (t & a)
            return -1;
        int n = countSetBits((t | a) & 0x7f);
        if (n > maximumAllocationParts) {
            reportError("nbits overrun \n");
            exitAfterCleanup(-1);
        }
        total += maximumAllocationParts - n;
    }
    if (total < 0)
        total = 0;
    return total;
}

bool genRegPool::set_t_in_aTab(int reg)
{
    int end = allocationTimelineEnd;
    for (int i = allocationTimelineStart; i < end; i++) {
        unsigned char c = candidateRegisterTimeline[i];
        int t = c;
        if (((unsigned char)unk_30[reg] & t & 0x7f) != (c & 0x7f)) {
            reportError(setTInATabAccessError, reg);
            return false;
        }
        char *row = &allocationRegisterTimeline[i * 32];
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
    for (int i = allocationTimelineStart; i != allocationTimelineEnd; i++)
        for (int r = 0; r != allocationRegisterCount; r++) {
            for (int p = 0; p != maximumAllocationParts; p++) {
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
    for (int r = 0; r != allocationRegisterCount; r++) {
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
    RegPoolEntry *reg = ru->registerEntry;
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
        if (reg->enabledPhysicalRegister) {
            unsigned char mask = ru->inputMask | ru->outputMask | ru->throughputMask;
            if (!mask) {
                reportError("error here\n");
                exitAfterCleanup(-1);
            }
            int set = 0;
            for (int i = 0; i != reg->partCount; i++) {
                bool on = (mask >> i) & 1;
                if (on && !reg->getPartPayload(i)) {
                    reportError("failed to get rll\n");
                    return false;
                }
                RegisterPartLockList *ll = (RegisterPartLockList *)reg->getPartPayload(i);
                if (ll) {
                    if (!ll->assigned)
                        set |= 1 << i;
                    ll->assigned = 1;
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
