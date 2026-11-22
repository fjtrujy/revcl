#include "os.h"
#include "lock_shared.h"

bool RegisterPartLockList::catLocks(OperandLock *firstLock, OperandLock *operandLock, int *rangeStartOut, int *rangeEndOut, int *end)
{
    bool found = false;
    int lsz = activeLoopSize;
    if (firstLock->scheduledConsumerCount) {
        *rangeStartOut = firstLock->earliestReadyCycle + lsz;
        *rangeEndOut = firstLock->fullLatencyReadyCycle + lsz;
        *end = firstLock->latestReadyCycle + lsz;
        found = true;
    }
    int lockReadyStart = operandLock->earliestReadyCycle;
    int fullLatencyStart = operandLock->fullLatencyReadyCycle;
    int bend = operandLock->latestReadyCycle;
    if (!found) {
        *rangeStartOut = lockReadyStart;
        *rangeEndOut = fullLatencyStart;
        *end = bend;
    } else {
        *rangeStartOut = lockReadyStart;
        *rangeEndOut = fullLatencyStart;
        if (bend > *end)
            *end = bend;
    }
    return true;
}

static inline bool isAAInputLock(OperandLock *operandLock)
{
    return operandLock->operandStatusCode == 2;
}

static inline bool sameLockOwner(OperandLock *firstLock, OperandLock *operandLock)
{
    return firstLock->ownerPointer == operandLock->ownerPointer;
}

int RegisterPartLockList::canMoveInputLock(OperandLock *operandLock, short *pos, int slotBit)
{
    int targetLinearRow = *pos;
    OperandLock *lock = outputLocks;
    int lsz = activeLoopSize;
    if (calculateLoopStats)
        return 0;
    if (isAAInputLock(operandLock)) {
        reportError("canMoveInputLock - aa input\n");
        abortAfterError();
        return 0;
    } else {
        OperandLock *par = operandLock->getParent();
        bool cat = false;
        int rangeStart, rangeEnd, end;
        if (lsz && isRegLock(par)) {
            OperandLock *externalOutputLock = externalOutputLocks[0];
            if (externalOutputLock->insertedIntoLockList) {
                if (sameLockOwner(operandLock, externalOutputLock))
                    return 0;
                targetLinearRow += lsz;
                if (!catLocks(par, externalOutputLock, &rangeStart, &rangeEnd, &end)) {
                    reportError("failed to concatenate reg ref with xo\n");
                    exitAfterCleanup(-1);
                }
                cat = true;
                par = externalOutputLock;
            } else if (par->scheduledConsumerCount) {
                rangeStart = par->earliestReadyCycle;
                rangeEnd = par->fullLatencyReadyCycle;
                end = par->latestReadyCycle;
                if (targetLinearRow - operandLock->issueLatency < rangeStart)
                    rangeStart = targetLinearRow - operandLock->issueLatency;
                if (targetLinearRow - operandLock->issueLatency < rangeEnd)
                    rangeEnd = targetLinearRow - operandLock->issueLatency;
                if (targetLinearRow > end)
                    end = targetLinearRow;
                if (end - rangeStart >= lsz)
                    return 0x2000;
            } else {
                rangeStart = targetLinearRow - operandLock->issueLatency;
                rangeEnd = targetLinearRow;
                end = targetLinearRow;
            }
        } else if (lsz && containsRegisterOutput && par->externalOutputSlotPlusOne) {
            if (!catLocks(sortedOutputLocks, par, &rangeStart, &rangeEnd, &end)) {
                reportError("case B failed to cat xo\n");
                checkCounts();
                reportError("we are :\n");
                operandLock->print();
                exitAfterCleanup(-1);
            }
            cat = true;
        } else {
            rangeStart = par->earliestReadyCycle;
            rangeEnd = par->fullLatencyReadyCycle;
            end = par->latestReadyCycle;
        }

        if (targetLinearRow - operandLock->issueLatency < rangeEnd)
            return 8;
        if (targetLinearRow <= end)
            return 0;
        end = targetLinearRow;
        if (lsz && end - rangeStart >= lsz)
            return 0x40000;
        int wrappedStart = rangeStart, wrappedEnd = end;
        wrapRangeLocal(wrappedStart, wrappedEnd);
        if (lock) do {
            lock->print();
            if (lock == par)
                continue;
            if (sameLockOwner(lock, operandLock))
                continue;
            if (cat && isRegLock(lock))
                continue;
            int lockStart = lock->adjustedEarliestReadyCycle;
            int lockEnd = lock->adjustedLatestReadyCycle;
            wrapRangeLocal(lockStart, lockEnd);
            if (lock->activeReferenceCount) {
                if (wrappedStart < lockEnd && wrappedEnd >= lockStart)
                    return 0x10;
            } else if (lockRangesOverlap(wrappedStart, wrappedEnd, lockStart, lockEnd))
                return 0x20;
        } while ((lock = lock->nextSibling) != 0);
    }
    return 0;
}

int RegisterPartLockList::canMoveOutputLock(OperandLock *outputLock, short *pos, int slotBit)
{
    int ret;
    int lsz = activeLoopSize;
    int targetLinearRow = ((InstructionPosition *)pos)->linearRow;
    int targetLoopRow = ((InstructionPosition *)pos)->loopRow;
    int overlapRow = targetLinearRow;
    ret = checkOverlap(outputLock, pos, &overlapRow);
    if (ret)
        return ret;
    int first = outputLock->latencyModeFlag ? targetLinearRow + 1 : outputLock->issueLatency + targetLinearRow;
    int last = outputLock->issueLatency + targetLinearRow;

    int lim;
    if (outputLock->referenceCount) {
        int upperConsumerCount = outputLock->upperPipeConsumerCount, lowerConsumerCount = outputLock->lowerPipeConsumerCount;
        int eitherConsumerCount = outputLock->eitherPipeConsumerCount;
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
    } else
        lim = first;

    if (outputLock->scheduledConsumerCount) {
        if (outputLock->latestConsumerCycle > lim)
            lim = outputLock->latestConsumerCycle;
        if (last > outputLock->earliestConsumerFreeCycle)
            return 2;
    }

    bool ext = false;
    if (outputLock->externalOutputSlotPlusOne) {
        for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
            if (lock == outputLock)
                continue;
            if (first <= lock->latestReadyCycle)
                return 0x1000;
        }
        if (lsz && containsRegisterOutput) {
            OperandLock *operandLock = sortedOutputLocks;
            ext = true;
            if (operandLock->scheduledConsumerCount) {
                int nextEarliestReadyCycle = operandLock->earliestReadyCycle + lsz;
                int nextLatestReadyCycle = operandLock->latestReadyCycle + lsz;
                if (nextEarliestReadyCycle < last)
                    return 2;
                if (nextLatestReadyCycle > lim)
                    lim = nextLatestReadyCycle;
            }
        }
    } else if (lsz && containsRegisterOutput) {
        OperandLock *firstOutputLock = sortedOutputLocks;
        if (firstOutputLock->scheduledConsumerCount) {
            if (first < firstOutputLock->latestReadyCycle)
                return 0x4000;
        } else
            ext = true;
    }
    if (lsz && lim - first > lsz)
        return 0x2000;

    int wrappedStart = targetLoopRow - targetLinearRow + first;
    int wrappedEnd = targetLoopRow - targetLinearRow + lim;
    wrapRangeLocal(wrappedStart, wrappedEnd);
    for (OperandLock *lock = outputLocks; lock; lock = lock->nextSibling) {
        if (lock == outputLock)
            continue;
        if (ext && lock->ownerType == 2)
            continue;
        int lockStart = lock->adjustedEarliestReadyCycle;
        int lockEnd = lock->adjustedLatestReadyCycle;
        wrapRangeLocal(lockStart, lockEnd);
        if (lockRangesOverlap(wrappedStart, wrappedEnd, lockStart, lockEnd))
            return 2;
    }
    if (ret)
        return ret;
    if (externalOutputCount && !lsz && !outputLock->latencyModeFlag) {
        OperandLock *externalOutputLock = externalOutputLocks[0];
        if (!externalOutputLock) {
            reportError("external lock not set \n");
            abortAfterError();
        }
        if (externalOutputLock->insertedIntoLockList) {
            if (outputLock->linearRow == targetLinearRow)
                return 0;
            if (targetLinearRow > externalOutputLock->linearRow)
                return 0x200000;
        }
    }
    return 0;
}

