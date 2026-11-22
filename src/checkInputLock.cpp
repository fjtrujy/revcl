#include "os.h"
#include "lock_shared.h"

bool allowInputLockLoopWrapping;

static inline bool canWrapInputLock(OperandLock *operandLock)
{
    return operandLock->operandStatusCode == 1;
}

static inline void wrapInputRange(int &rangeStart, int &rangeEnd)
{
    int loopPeriod = activeLoopSize;
    if (loopPeriod) {
        if (rangeStart >= loopPeriod) {
            do {
                rangeStart -= loopPeriod;
                rangeEnd -= loopPeriod;
            } while (rangeStart >= loopPeriod);
        }
    }
}

int cls0805c244::checkInputLock(OperandLock *operandLock, short *pos, int *minPos)
{
    OperandLock *lock = outputLocks;
    int ret;
    int lsz = activeLoopSize;
    ret = 0;
    int targetLinearRow = ((InstructionPosition *)pos)->linearRow;

    operandLock->priority = 0;
    if (operandLock->operandStatusCode == 2) {
        reportError("CheckInputLock::aa input\n");
        abortAfterError();
        return 0;
    }
    if (calculateLoopStats)
        return 0;

    OperandLock *par = operandLock->getParent();
    int parentEarliestReady = par->earliestReadyCycle;
    int parentLatestReady = par->latestReadyCycle;

    if (lsz && isRegLock(par)) {
        if (par->scheduledConsumerCount == 0)
            return 0;
        bool inside = true;
        if (targetLinearRow < parentEarliestReady) { parentEarliestReady = targetLinearRow; inside = false; }
        if (targetLinearRow > parentLatestReady) { parentLatestReady = targetLinearRow; inside = false; }
        if (!inside) {
            int span = parentLatestReady - parentEarliestReady;
            int room = lsz;
            if (externalOutputLocks[0])
                room -= externalOutputLocks[0]->issueLatency;
            else {
                reportError("no ext output set. but .. not const for loop\n");
                exitAfterCleanup(-1);
            }
            if (span > room) {
                int need;
                if (targetLinearRow < par->earliestConsumerFreeCycle)
                    need = par->latestReadyCycle - room;
                else if (canWrapInputLock(operandLock)) {
                    operandLock->priority = 1;
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

    int start = par->fullLatencyReadyCycle + operandLock->issueLatency;
    if (targetLinearRow < start) {
        if (start > *minPos)
            *minPos = start;
        return 8;
    }
    if (targetLinearRow <= parentLatestReady)
        return 0;

    int schedulingDistance = targetLinearRow - parentEarliestReady;
    if (lsz && schedulingDistance >= lsz) {
        if (canWrapInputLock(operandLock)) {
            operandLock->priority = 3;
            return 0x100;
        }
        *minPos = 30000;
        return 0x400;
    }

    int wrappedStart = par->adjustedEarliestReadyCycle;
    int wrappedEnd = wrappedStart + schedulingDistance;
    wrapInputRange(wrappedStart, wrappedEnd);

    bool isExt = false;
    if (lsz && par == externalOutputLocks[0])
        isExt = true;

    for (; lock; lock = lock->nextSibling) {
        if (lock == par)
            continue;
        if (isExt && lock->ownerType == 2)
            continue;
        int lockStart = lock->adjustedEarliestReadyCycle;
        int lockEnd = lock->adjustedLatestReadyCycle;
        if (lock->activeReferenceCount) {
            if (wrappedStart < lockEnd && wrappedEnd >= lockStart) {
                *minPos = 30000;
                return ret | 0x10;
            }
        } else if (lockRangesOverlap(wrappedStart, wrappedEnd, lockStart, lockEnd)) {
            if (allowInputLockLoopWrapping && lsz && canWrapInputLock(operandLock)) {
                operandLock->priority = 2;
                ret |= 0x100;
            } else {
                *minPos = 30000;
                return ret | 0x20;
            }
        }
    }
    return ret;
}

static inline void nextLoopPosition(int requestedRow, int row, int loopPeriod, int &next)
{
    if (!loopPeriod)
        next = requestedRow;
    else {
        int loopRemainder = requestedRow % loopPeriod;
        int rem = row % loopPeriod;
        int loopQuotient = row / loopPeriod;
        if (loopRemainder <= rem)
            loopQuotient++;
        next = loopQuotient * loopPeriod + loopRemainder;
    }
}

int cls0805c244::checkOutputLock(OperandLock *out, short *pos, int *minPos)
{
    int lsz = activeLoopSize;
    int ret = checkOverlap(out, pos, minPos);
    if (isRegLock(out))
        return 0;
    int targetLinearRow = ((InstructionPosition *)pos)->linearRow;
    int first;
    if (out->latencyModeFlag)
        first = targetLinearRow + 1;
    else
        first = out->issueLatency + targetLinearRow;
    int last = out->issueLatency + targetLinearRow;

    if (out->externalOutputSlotPlusOne) {
        if (remainingLockCount > 1) {
            *minPos = 30000;
            return ret | 0x800;
        }
        for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
            if (first <= lock->latestReadyCycle) {
                int minimumRequiredRow = lock->latestReadyCycle - (first - targetLinearRow) + 1;
                if (*minPos < minimumRequiredRow)
                    *minPos = minimumRequiredRow;
                ret |= 0x1000;
            }
        }
        if (ret)
            return ret;
    }

    bool ext = false;
    int lim;
    int nextLatestReadyCycle;
    if (!out->referenceCount)
        lim = first;
    else {
        int upperConsumerCount = out->upperPipeConsumerCount, lowerConsumerCount = out->lowerPipeConsumerCount;
        int eitherConsumerCount = out->eitherPipeConsumerCount;
        int maxConsumerCount = upperConsumerCount, minConsumerCount = lowerConsumerCount;
        if (upperConsumerCount < lowerConsumerCount) {
            maxConsumerCount = lowerConsumerCount;
            minConsumerCount = upperConsumerCount;
        }
        eitherConsumerCount -= maxConsumerCount - minConsumerCount;
        if (eitherConsumerCount > 0)
            maxConsumerCount += (eitherConsumerCount + 1) >> 1;
        int off = maxConsumerCount;
        if (maxConsumerCount > 0)
            off = maxConsumerCount - 1;
        lim = last + off;
    }

    if (lsz && containsRegisterOutput) {
        OperandLock *operandLock = sortedOutputLocks;
        if (operandLock->activeReferenceCount) {
            *minPos = 30000;
            return ret | 4;
        }
        if (operandLock->scheduledConsumerCount) {
            if (first <= operandLock->latestReadyCycle) {
                int candidateRequiredRow = operandLock->latestReadyCycle - (first - targetLinearRow) + 1;
                if (*minPos < candidateRequiredRow)
                    *minPos = candidateRequiredRow;
                ret |= 0x4000;
            }
            int nextEarliestReadyCycle = operandLock->earliestConsumerFreeCycle + lsz;
            nextLatestReadyCycle = operandLock->latestReadyCycle + lsz;
            if (nextEarliestReadyCycle < last) {
                *minPos = 30000;
                return 0x20000;
            }
            if (out->externalOutputSlotPlusOne) {
                if (nextLatestReadyCycle - first >= lsz) {
                    *minPos = 30000;
                    return 0x2000;
                }
                if (nextLatestReadyCycle > lim)
                    lim = nextLatestReadyCycle;
                ext = true;
            }
        } else
            ext = true;
    }

    int targetLoopRow = ((InstructionPosition *)pos)->loopRow;
    int wrappedStart = targetLoopRow + (first - targetLinearRow);
    int wrappedEnd = targetLoopRow + (lim - targetLinearRow);
    wrapRangeLocal(wrappedStart, wrappedEnd);

    for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
        if (ext && lock->ownerType == 2)
            continue;
        int lockStart = lock->adjustedEarliestReadyCycle;
        int lockEnd = lock->adjustedLatestReadyCycle;
        wrapRangeLocal(lockStart, lockEnd);
        if (lock->activeReferenceCount) {
            if (lsz) {
                *minPos = 30000;
                return ret | 4;
            }
            if (wrappedEnd >= lockStart) {
                *minPos = 30000;
                return ret | 4;
            }
        } else if (lockRangesOverlap(wrappedStart, wrappedEnd, lockStart, lockEnd)) {
            first -= targetLinearRow;
            int nextRequiredCycle = lock->latestReadyCycle - first + 1;
            nextLoopPosition(nextRequiredCycle, ((InstructionPosition *)pos)->linearRow, activeLoopSize, nextLatestReadyCycle);
            if (*minPos < nextLatestReadyCycle)
                *minPos = nextLatestReadyCycle;
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

    int overlapStart = pos[2];
    int overlapEnd = overlapStart + out->throughputSpacing - 1;
    for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
        if (lock == out)
            continue;
        int otherLockStart = lock->loopRow;
        int otherLockEnd = otherLockStart + lock->throughputSpacing;
        if (otherLockEnd - 1 < 0)
            continue;
        if (lockRangesOverlap(overlapStart, overlapEnd, otherLockStart, otherLockEnd - 1)) {
            ret |= 1;
            int targetLinearRow = ((InstructionPosition *)pos)->linearRow;
            int nextRequiredRow;
            if (!activeLoopSize)
                nextRequiredRow = otherLockEnd;
            else {
                int loopRemainder = otherLockEnd % activeLoopSize;
                int loopQuotient = targetLinearRow / activeLoopSize;
                if (loopRemainder <= targetLinearRow % activeLoopSize)
                    loopQuotient++;
                nextRequiredRow = loopQuotient * activeLoopSize + loopRemainder;
            }
            if (*minPos < nextRequiredRow)
                *minPos = nextRequiredRow;
        }
    }
    return ret;
}

bool lockRangesOverlap(int s1, int e1, int s2, int e2)
{
    int loopPeriod = activeLoopSize;
    if (loopPeriod) {
        int l1 = e1 - s1;
        int l2 = e2 - s2;
        wrapRangeLocal(s1, e1);
        wrapRangeLocal(s2, e2);
        if (l1 + l2 > loopPeriod)
            return true;
        if (s1 <= s2)
            return e1 >= s2 || e2 - loopPeriod >= s1;
        return s1 <= e2 || e1 - loopPeriod >= s2;
    }
    if (s1 <= s2)
        return e1 >= s2;
    return s1 <= e2;
}
