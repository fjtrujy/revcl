#include "os.h"
#include "lock_shared.h"

bool g_080de288;

static inline bool canWrapInputLock(OperandLock *in)
{
    return in->unk_19 == 1;
}

static inline void wrapInputRange(int &s, int &e)
{
    int n = activeLoopSize;
    if (n) {
        if (s >= n) {
            do {
                s -= n;
                e -= n;
            } while (s >= n);
        }
    }
}

int cls0805c244::checkInputLock(OperandLock *in, short *pos, int *minPos)
{
    OperandLock *lk = outputLocks;
    int ret;
    int lsz = activeLoopSize;
    ret = 0;
    int p = ((InstructionPosition *)pos)->linearRow;

    in->priority = 0;
    if (in->unk_19 == 2) {
        reportError("CheckInputLock::aa input\n");
        abortAfterError();
        return 0;
    }
    if (calculateLoopStats)
        return 0;

    OperandLock *par = in->getParent();
    int lo = par->earliestReadyCycle;
    int hi = par->latestReadyCycle;

    if (lsz && isRegLock(par)) {
        if (par->scheduledConsumerCount == 0)
            return 0;
        bool inside = true;
        if (p < lo) { lo = p; inside = false; }
        if (p > hi) { hi = p; inside = false; }
        if (!inside) {
            int span = hi - lo;
            int room = lsz;
            if (externalOutputLocks[0])
                room -= externalOutputLocks[0]->issueLatency;
            else {
                reportError("no ext output set. but .. not const for loop\n");
                exitAfterCleanup(-1);
            }
            if (span > room) {
                int need;
                if (p < par->earliestConsumerFreeCycle)
                    need = par->latestReadyCycle - room;
                else if (canWrapInputLock(in)) {
                    in->priority = 1;
                    return 0x100;
                } else
                    need = 30000;
                if (*minPos < need)
                    *minPos = need;
                return 0x2000;
            }
        }
        return 0;
    }

    int start = par->fullLatencyReadyCycle + in->issueLatency;
    if (p < start) {
        if (start > *minPos)
            *minPos = start;
        return 8;
    }
    if (p <= hi)
        return 0;

    int d = p - lo;
    if (lsz && d >= lsz) {
        if (canWrapInputLock(in)) {
            in->priority = 3;
            return 0x100;
        }
        *minPos = 30000;
        return 0x400;
    }

    int s = par->adjustedEarliestReadyCycle;
    int e = s + d;
    wrapInputRange(s, e);

    bool isExt = false;
    if (lsz && par == externalOutputLocks[0])
        isExt = true;

    for (; lk; lk = lk->nextSibling) {
        if (lk == par)
            continue;
        if (isExt && lk->ownerType == 2)
            continue;
        int ls = lk->adjustedEarliestReadyCycle;
        int le = lk->adjustedLatestReadyCycle;
        if (lk->activeReferenceCount) {
            if (s < le && e >= ls) {
                *minPos = 30000;
                return ret | 0x10;
            }
        } else if (lockRangesOverlap(s, e, ls, le)) {
            if (g_080de288 && lsz && canWrapInputLock(in)) {
                in->priority = 2;
                ret |= 0x100;
            } else {
                *minPos = 30000;
                return ret | 0x20;
            }
        }
    }
    return ret;
}

static inline void nextLoopPosition(int v, int row, int n, int &next)
{
    if (!n)
        next = v;
    else {
        int r = v % n;
        int rem = row % n;
        int q = row / n;
        if (r <= rem)
            q++;
        next = q * n + r;
    }
}

int cls0805c244::checkOutputLock(OperandLock *out, short *pos, int *minPos)
{
    int lsz = activeLoopSize;
    int ret = checkOverlap(out, pos, minPos);
    if (isRegLock(out))
        return 0;
    int p = ((InstructionPosition *)pos)->linearRow;
    int first;
    if (out->latencyModeFlag)
        first = p + 1;
    else
        first = out->issueLatency + p;
    int last = out->issueLatency + p;

    if (out->externalOutputSlotPlusOne) {
        if (remainingLockCount > 1) {
            *minPos = 30000;
            return ret | 0x800;
        }
        for (OperandLock *lk = outputLocks; lk; lk = lk->nextSibling) {
            if (first <= lk->latestReadyCycle) {
                int v = lk->latestReadyCycle - (first - p) + 1;
                if (*minPos < v)
                    *minPos = v;
                ret |= 0x1000;
            }
        }
        if (ret)
            return ret;
    }

    bool ext = false;
    int lim;
    int he;
    if (!out->referenceCount)
        lim = first;
    else {
        int a = out->unk_48, b = out->unk_4a;
        int d = out->unk_46;
        int hi = a, lo = b;
        if (a < b) {
            hi = b;
            lo = a;
        }
        d -= hi - lo;
        if (d > 0)
            hi += (d + 1) >> 1;
        int off = hi;
        if (hi > 0)
            off = hi - 1;
        lim = last + off;
    }

    if (lsz && containsRegisterOutput) {
        OperandLock *h = sortedOutputLocks;
        if (h->activeReferenceCount) {
            *minPos = 30000;
            return ret | 4;
        }
        if (h->scheduledConsumerCount) {
            if (first <= h->latestReadyCycle) {
                int v = h->latestReadyCycle - (first - p) + 1;
                if (*minPos < v)
                    *minPos = v;
                ret |= 0x4000;
            }
            int hs = h->earliestConsumerFreeCycle + lsz;
            he = h->latestReadyCycle + lsz;
            if (hs < last) {
                *minPos = 30000;
                return 0x20000;
            }
            if (out->externalOutputSlotPlusOne) {
                if (he - first >= lsz) {
                    *minPos = 30000;
                    return 0x2000;
                }
                if (he > lim)
                    lim = he;
                ext = true;
            }
        } else
            ext = true;
    }

    int sz = ((InstructionPosition *)pos)->loopRow;
    int s = sz + (first - p);
    int e = sz + (lim - p);
    wrapRangeLocal(s, e);

    for (OperandLock *lk = outputLocks; lk; lk = lk->nextSibling) {
        if (ext && lk->ownerType == 2)
            continue;
        int ls = lk->adjustedEarliestReadyCycle;
        int le = lk->adjustedLatestReadyCycle;
        wrapRangeLocal(ls, le);
        if (lk->activeReferenceCount) {
            if (lsz) {
                *minPos = 30000;
                return ret | 4;
            }
            if (e >= ls) {
                *minPos = 30000;
                return ret | 4;
            }
        } else if (lockRangesOverlap(s, e, ls, le)) {
            first -= p;
            int v = lk->latestReadyCycle - first + 1;
            nextLoopPosition(v, ((InstructionPosition *)pos)->linearRow, activeLoopSize, he);
            if (*minPos < he)
                *minPos = he;
            return ret | 2;
        }
    }
    return ret;
}

int cls0805c244::checkOverlap(OperandLock *out, short *pos, int *minPos)
{
    int ret = 0;
    if (out->throughputSpacing <= 1)
        return 0;

    int s = pos[2];
    int e = s + out->throughputSpacing - 1;
    for (OperandLock *lk = outputLocks; lk; lk = lk->nextSibling) {
        if (lk == out)
            continue;
        int os = lk->loopRow;
        int oe = os + lk->throughputSpacing;
        if (oe - 1 < 0)
            continue;
        if (lockRangesOverlap(s, e, os, oe - 1)) {
            ret |= 1;
            int p = ((InstructionPosition *)pos)->linearRow;
            int v;
            if (!activeLoopSize)
                v = oe;
            else {
                int r = oe % activeLoopSize;
                int q = p / activeLoopSize;
                if (r <= p % activeLoopSize)
                    q++;
                v = q * activeLoopSize + r;
            }
            if (*minPos < v)
                *minPos = v;
        }
    }
    return ret;
}

bool lockRangesOverlap(int s1, int e1, int s2, int e2)
{
    int n = activeLoopSize;
    if (n) {
        int l1 = e1 - s1;
        int l2 = e2 - s2;
        wrapRangeLocal(s1, e1);
        wrapRangeLocal(s2, e2);
        if (l1 + l2 > n)
            return true;
        if (s1 <= s2)
            return e1 >= s2 || e2 - n >= s1;
        return s1 <= e2 || e1 - n >= s2;
    }
    if (s1 <= s2)
        return e1 >= s2;
    return s1 <= e2;
}
