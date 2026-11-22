#include "os.h"
#include "lock_shared.h"

bool cls0805c244::catLocks(OperandLock *firstLock, OperandLock *b, int *s, int *e, int *end)
{
    bool found = false;
    int lsz = activeLoopSize;
    if (firstLock->scheduledConsumerCount) {
        *s = firstLock->earliestReadyCycle + lsz;
        *e = firstLock->fullLatencyReadyCycle + lsz;
        *end = firstLock->latestReadyCycle + lsz;
        found = true;
    }
    int bs = b->earliestReadyCycle;
    int be = b->fullLatencyReadyCycle;
    int bend = b->latestReadyCycle;
    if (!found) {
        *s = bs;
        *e = be;
        *end = bend;
    } else {
        *s = bs;
        *e = be;
        if (bend > *end)
            *end = bend;
    }
    return true;
}

static inline bool isAAInputLock(OperandLock *in)
{
    return in->operandStatusCode == 2;
}

static inline bool sameLockOwner(OperandLock *firstLock, OperandLock *b)
{
    return firstLock->ownerPointer == b->ownerPointer;
}

int cls0805c244::canMoveInputLock(OperandLock *in, short *pos, int slotBit)
{
    int p = *pos;
    OperandLock *lock = outputLocks;
    int lsz = activeLoopSize;
    if (calculateLoopStats)
        return 0;
    if (isAAInputLock(in)) {
        reportError("canMoveInputLock - aa input\n");
        abortAfterError();
        return 0;
    } else {
        OperandLock *par = in->getParent();
        bool cat = false;
        int s, e, end;
        if (lsz && isRegLock(par)) {
            OperandLock *x = externalOutputLocks[0];
            if (x->insertedIntoLockList) {
                if (sameLockOwner(in, x))
                    return 0;
                p += lsz;
                if (!catLocks(par, x, &s, &e, &end)) {
                    reportError("failed to concatenate reg ref with xo\n");
                    exitAfterCleanup(-1);
                }
                cat = true;
                par = x;
            } else if (par->scheduledConsumerCount) {
                s = par->earliestReadyCycle;
                e = par->fullLatencyReadyCycle;
                end = par->latestReadyCycle;
                if (p - in->issueLatency < s)
                    s = p - in->issueLatency;
                if (p - in->issueLatency < e)
                    e = p - in->issueLatency;
                if (p > end)
                    end = p;
                if (end - s >= lsz)
                    return 0x2000;
            } else {
                s = p - in->issueLatency;
                e = p;
                end = p;
            }
        } else if (lsz && containsRegisterOutput && par->externalOutputSlotPlusOne) {
            if (!catLocks(sortedOutputLocks, par, &s, &e, &end)) {
                reportError("case B failed to cat xo\n");
                checkCounts();
                reportError("we are :\n");
                in->print();
                exitAfterCleanup(-1);
            }
            cat = true;
        } else {
            s = par->earliestReadyCycle;
            e = par->fullLatencyReadyCycle;
            end = par->latestReadyCycle;
        }

        if (p - in->issueLatency < e)
            return 8;
        if (p <= end)
            return 0;
        end = p;
        if (lsz && end - s >= lsz)
            return 0x40000;
        int ws = s, we = end;
        wrapRangeLocal(ws, we);
        if (lock) do {
            lock->print();
            if (lock == par)
                continue;
            if (sameLockOwner(lock, in))
                continue;
            if (cat && isRegLock(lock))
                continue;
            int ls = lock->adjustedEarliestReadyCycle;
            int le = lock->adjustedLatestReadyCycle;
            wrapRangeLocal(ls, le);
            if (lock->activeReferenceCount) {
                if (ws < le && we >= ls)
                    return 0x10;
            } else if (lockRangesOverlap(ws, we, ls, le))
                return 0x20;
        } while ((lock = lock->nextSibling) != 0);
    }
    return 0;
}

int cls0805c244::canMoveOutputLock(OperandLock *out, short *pos, int slotBit)
{
    int ret;
    int lsz = activeLoopSize;
    int p = ((InstructionPosition *)pos)->linearRow;
    int sz = ((InstructionPosition *)pos)->loopRow;
    int mp = p;
    ret = checkOverlap(out, pos, &mp);
    if (ret)
        return ret;
    int first = out->latencyModeFlag ? p + 1 : out->issueLatency + p;
    int last = out->issueLatency + p;

    int lim;
    if (out->referenceCount) {
        int a = out->upperPipeConsumerCount, b = out->lowerPipeConsumerCount;
        int d = out->eitherPipeConsumerCount;
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
    } else
        lim = first;

    if (out->scheduledConsumerCount) {
        if (out->latestConsumerCycle > lim)
            lim = out->latestConsumerCycle;
        if (last > out->earliestConsumerFreeCycle)
            return 2;
    }

    bool ext = false;
    if (out->externalOutputSlotPlusOne) {
        for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
            if (lock == out)
                continue;
            if (first <= lock->latestReadyCycle)
                return 0x1000;
        }
        if (lsz && containsRegisterOutput) {
            OperandLock *h = sortedOutputLocks;
            ext = true;
            if (h->scheduledConsumerCount) {
                int hs = h->earliestReadyCycle + lsz;
                int he = h->latestReadyCycle + lsz;
                if (hs < last)
                    return 2;
                if (he > lim)
                    lim = he;
            }
        }
    } else if (lsz && containsRegisterOutput) {
        OperandLock *h = sortedOutputLocks;
        if (h->scheduledConsumerCount) {
            if (first < h->latestReadyCycle)
                return 0x4000;
        } else
            ext = true;
    }
    if (lsz && lim - first > lsz)
        return 0x2000;

    int s = sz - p + first;
    int e = sz - p + lim;
    wrapRangeLocal(s, e);
    for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
        if (lock == out)
            continue;
        if (ext && lock->ownerType == 2)
            continue;
        int ls = lock->adjustedEarliestReadyCycle;
        int le = lock->adjustedLatestReadyCycle;
        wrapRangeLocal(ls, le);
        if (lockRangesOverlap(s, e, ls, le))
            return 2;
    }
    if (ret)
        return ret;
    if (externalOutputCount && !lsz && !out->latencyModeFlag) {
        OperandLock *x = externalOutputLocks[0];
        if (!x) {
            reportError("external lock not set \n");
            abortAfterError();
        }
        if (x->insertedIntoLockList) {
            if (out->linearRow == p)
                return 0;
            if (p > x->linearRow)
                return 0x200000;
        }
    }
    return 0;
}

