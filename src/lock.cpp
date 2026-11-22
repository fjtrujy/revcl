#include "Instr.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"
#include "Block.h"
#include "statCounts.h"

extern int errorOccurred;
extern bool quietDiagnostics;
bool fatalReadBeforeReady;

bool cls0805bcc4::resetLock()
{
    activeReferenceCount = 0;
    referenceCount = 0;
    parentOrChildren = 0;
    nextSibling = 0;
    externalOutputSlotPlusOne = 0;
    throughputSpacing = 0;
    issueLatency = 0;
    latencyModeFlag = 0;
    pairedLock = 0;
    insertedIntoLockList = 0;
    operandStatusCode = 0;
    nextSortedOutput = 0;
    scheduledConsumerCount = 0;
    upperPipeConsumerCount = 0;
    lowerPipeConsumerCount = 0;
    eitherPipeConsumerCount = 0;
    eitherPipeInputDemand = 0;
    upperPipeInputDemand = 0;
    lowerPipeInputDemand = 0;
    ownerPointer = 0;
    ownerType = 0;
    lockDirection = 0;
    ownerLockList = 0;
    ownerComponent = -1;
    ownerOperandIndex = -1;
    linearRow = -1;
    loopRow = -1;
    adjustedEarliestReadyCycle = -1;
    earliestReadyCycle = -1;
    adjustedFullLatencyReadyCycle = -1;
    fullLatencyReadyCycle = -1;
    adjustedEarliestConsumerFreeCycle = -1;
    earliestConsumerFreeCycle = -1;
    adjustedLatestConsumerCycle = -1;
    latestConsumerCycle = -1;
    latestReadyCycle = -1;
    adjustedLatestReadyCycle = -1;
    return true;
}

bool cls0805bcc4::setOwner(int part, Instr *instr, int opnd)
{
    if (!instr) {
        reportError("setowner - passed null\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (ownerPointer) {
        reportError("owner already set for rlock set owner\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (ownerType) {
        reportError("owner: type wasn't null when try to set\n");
        return false;
    }
    if (ownerLockList) {
        reportError("setowner - lock was already on list\n");
        exitAfterCleanup(-1);
        return false;
    }
    ownerType = 1;
    ownerComponent = part;
    ownerOperandIndex = opnd;
    ownerPointer = instr;
    return true;
}

bool cls0805bcc4::setOwner(int part, regUse *registerUse)
{
    if (!registerUse) {
        reportError("setowner - passed null rf\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (ownerPointer) {
        reportError("owner already set for rlock set owner\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (ownerType) {
        reportError("owner: type wasn't null when try to set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (ownerLockList) {
        reportError("setowner - lock was already on list\n");
        exitAfterCleanup(-1);
        return false;
    }
    ownerOperandIndex = 0;
    ownerComponent = part;
    unk_2a = 0;
    ownerType = 2;
    ownerPointer = registerUse;
    linearRow = -1;
    loopRow = -1;
    return true;
}

bool cls0805bcc4::clearOwner()
{
    ownerPointer = 0;
    ownerType = 0;
    lockDirection = 0;
    ownerLockList = 0;
    ownerComponent = -1;
    ownerOperandIndex = -1;
    return true;
}

Instr *cls0805bcc4::ownerInstr()
{
    if (ownerType == 1)
        return (Instr *)ownerPointer;
    return 0;
}

regUse *cls0805bcc4::ownerRegUse()
{
    if (ownerType == 2)
        return (regUse *)ownerPointer;
    return 0;
}

Operand *cls0805bcc4::ownerOperand()
{
    if (ownerType == 1) {
        int opnd = ownerOperandIndex;
        Instr *instr = (Instr *)ownerPointer;
        return &instr->operands[opnd];
    }
    return 0;
}

int cls0805bcc4::ownerOpnd()
{
    if (ownerType == 1)
        return ownerOperandIndex;
    return -1;
}

int cls0805bcc4::getPart()
{
    if (ownerType == 1 || ownerType == 2)
        return ownerComponent;
    reportError("failed to get part - lock incorrect oType\n");
    exitAfterCleanup(-1);
    return -1;
}

void cls0805bcc4::printOwner()
{
    switch (ownerType) {
    case 0:
        break;
    case 1: {
        Instr *instr = ownerType == 1 ? (Instr *)ownerPointer : 0;
        if (!instr) {
            reportError("type inst..  can't get it though!\n");
            exitAfterCleanup(-1);
        }
        instr->dumpOperandDiagnostics(false);
        break;
    }
    case 2: {
        regUse *ownerUse = ownerType == 2 ? (regUse *)ownerPointer : 0;
        if (!ownerUse) {
            reportError("type regref..  can't get it though!\n");
            exitAfterCleanup(-1);
        }
        break;
    }
    default:
        reportError("??????????? unkown lock\n");
        exitAfterCleanup(-1);
    }
}

cls0805bcc4 *cls0805bcc4::getParent()
{
    if (lockDirection != 1) {
        reportError("getParent - called on non input lock\n");
        print();
        return 0;
    }
    if (!parentOrChildren) {
        reportWarning("getParent: not set!\n");
        print();
        return 0;
    }
    if (parentOrChildren->lockDirection != 2) {
        reportError("parent was not output!\n");
        reportError("we are:\n");
        print();
        reportError("parent is\n");
        parentOrChildren->print();
        exitAfterCleanup(-1);
    }
    return parentOrChildren;
}

bool cls0805bcc4::setInOut(cls0805bcc4 *olk)
{
    if (!olk) {
        reportError("setinout - passed null\n");
        exitAfterCleanup(-1);
    }
    if (olk->lockDirection != 2) {
        reportError("setinout - olk wasn't output\n");
        exitAfterCleanup(-1);
    }
    if (!ownerPointer) {
        reportError("owner not set for inout\n");
        exitAfterCleanup(-1);
    }
    if (ownerPointer != olk->ownerPointer) {
        reportError("setinout -- inst bad\n");
        exitAfterCleanup(-1);
    }
    if (ownerComponent != olk->ownerComponent) {
        reportError("set in out - part mismatched\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (pairedLock) {
        reportError("rlock - set inout - already set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lockDirection != 1) {
        reportError("set inout on non input\n");
        print();
        exitAfterCleanup(-1);
        return false;
    }
    pairedLock = olk;
    return true;
}

bool cls0805bcc4::incRef()
{
    activeReferenceCount++;
    return true;
}

bool cls0805bcc4::decRef()
{
    activeReferenceCount--;
    return true;
}

void cls0805bcc4::print()
{
    switch (lockDirection) {
    case 0:
    case 1:
        break;
    case 2:
        for (cls0805bcc4 *lock = parentOrChildren; lock; lock = lock->nextSibling)
            lock->print();
        break;
    }
    printOwner();
}

cls0805c244::cls0805c244()
{
    registerEntry = 0;
    nextLockList = 0;
    for (int externalOutputIndex = 0; externalOutputIndex != 4; externalOutputIndex++)
        externalOutputLocks[externalOutputIndex] = 0;
    externalOutputCount = 0;
    sharedExternalOutputFlag = 0;
    outputLocks = 0;
    outputLockCount = 0;
    remainingLockCount = 0;
    containsRegisterOutput = 0;
    calculateLoopStats = 1;
    sortedOutputLocks = 0;
    loopWrapMinimumRow = 0;
}

cls0805c244::cls0805c244(cls0805c244 *next)
{
    nextLockList = next;
    registerEntry = 0;
    for (int externalOutputIndex = 0; externalOutputIndex != 4; externalOutputIndex++)
        externalOutputLocks[externalOutputIndex] = 0;
    externalOutputCount = 0;
    sharedExternalOutputFlag = 0;
    outputLocks = 0;
    outputLockCount = 0;
    remainingLockCount = 0;
    containsRegisterOutput = 0;
    calculateLoopStats = 1;
    sortedOutputLocks = 0;
    loopWrapMinimumRow = 0;
}

cls0805c244::~cls0805c244()
{
}

void cls0805c244::checkCounts()
{
    int siblingCount = 0;
    for (cls0805bcc4 *outputLock = outputLocks; outputLock; outputLock = outputLock->nextSibling) {
        outputLock->printOwner();
        for (cls0805bcc4 *lock = outputLock->parentOrChildren; lock; lock = lock->nextSibling)
            lock->printOwner();
        siblingCount++;
    }
    if (outputLockCount != remainingLockCount + siblingCount)
        reportError("ERROR - nOutLocks != nToGo + nDone\n");
}

bool cls0805c244::reset()
{
    for (int externalOutputIndex = 0; externalOutputIndex != 4; externalOutputIndex++)
        externalOutputLocks[externalOutputIndex] = 0;
    externalOutputCount = 0;
    sharedExternalOutputFlag = 0;
    outputLocks = 0;
    outputLockCount = 0;
    remainingLockCount = 0;
    containsRegisterOutput = 0;
    calculateLoopStats = 1;
    sortedOutputLocks = 0;
    loopWrapMinimumRow = 0;
    return true;
}

bool cls0805c244::addInput(cls0805bcc4 *out, cls0805bcc4 *inputLock, char issueLatency, unsigned char latencyModeFlag)
{
    inputLock->lockDirection = 1;
    inputLock->ownerLockList = this;
    out->latencyModeFlag = latencyModeFlag;
    inputLock->issueLatency = issueLatency;
    out->eitherPipeConsumerCount += inputLock->eitherPipeInputDemand;
    out->upperPipeConsumerCount += inputLock->upperPipeInputDemand;
    out->referenceCount++;
    out->activeReferenceCount++;
    inputLock->parentOrChildren = out;
    out->lowerPipeConsumerCount += inputLock->lowerPipeInputDemand;
    return true;
}

static inline bool isRegLock(cls0805bcc4 *lock)
{
    return lock->ownerType == 2;
}

bool cls0805c244::addOutput(cls0805bcc4 *lock, int throughputSpacing, int issueLatency, unsigned char latencyModeFlag)
{
    if (isRegLock(lock))
        containsRegisterOutput = 1;
    lock->lockDirection = 2;
    lock->throughputSpacing = throughputSpacing;
    lock->issueLatency = issueLatency;
    lock->ownerLockList = this;
    lock->latencyModeFlag = latencyModeFlag;
    remainingLockCount++;
    outputLockCount++;
    return true;
}

bool cls0805c244::removeInput(cls0805bcc4 *inputLock)
{
    cls0805bcc4 *out = inputLock->parentOrChildren;
    out->eitherPipeConsumerCount -= inputLock->eitherPipeInputDemand;
    out->upperPipeConsumerCount -= inputLock->upperPipeInputDemand;
    out->referenceCount--;
    out->activeReferenceCount--;
    out->lowerPipeConsumerCount -= inputLock->lowerPipeInputDemand;
    inputLock->ownerLockList = 0;
    inputLock->lockDirection = 0;
    inputLock->parentOrChildren = 0;
    return true;
}

bool cls0805c244::removeOutput(cls0805bcc4 *lock)
{
    lock->lockDirection = 0;
    lock->ownerLockList = 0;
    remainingLockCount--;
    outputLockCount--;
    return true;
}

static inline bool isOwnedBy(cls0805bcc4 *lock, cls0805c244 *lockList)
{
    return lock->ownerLockList == lockList;
}

bool cls0805c244::addOutputLock(cls0805bcc4 *lock)
{
    if (!lock) {
        reportError("Addoutputlock - null pointer\n");
        return false;
    }
    if (lock->lockDirection != 2) {
        reportError("Addoutputlock - bad type\n");
        return false;
    }
    if (lock->insertedIntoLockList) {
        reportError("Addoutputlock - already inserted\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (!isOwnedBy(lock, this)) {
        reportError("Addoutput lock - I don't own it\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lock->parentOrChildren) {
        reportError("AddOutputLock: m_Link already set!\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lock->activeReferenceCount != lock->referenceCount) {
        reportError("Addoutputlock - mRefCount!=m_RefCount\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lock->nextSibling) {
        reportError("AddOutputLock:: m_Next already set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (remainingLockCount <= 0) {
        reportError("add output lock - count was already down to zero or less\n");
        checkCounts();
        exitAfterCleanup(-1);
        return false;
    }
    remainingLockCount--;
    lock->nextSibling = outputLocks;
    outputLocks = lock;
    if (!lock->calcStats(0)) {
        reportError("AddOutputLock failed to calcStats\n");
        checkCounts();
        exitAfterCleanup(-1);
    }
    lock->insertedIntoLockList = 1;
    lock->nextSortedOutput = 0;
    cls0805bcc4 *prev = 0;
    for (cls0805bcc4 *cur = sortedOutputLocks; cur; cur = cur->nextSortedOutput) {
        int lockLoopRow = lock->loopRow;
        int crl = cur->loopRow;
        if ((short)lockLoopRow == (short)crl && (short)lockLoopRow >= 0) {
            reportError("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            exitAfterCleanup(-1);
        }
        int line = lock->linearRow;
        int cline = cur->linearRow;
        if ((short)line < (short)cline)
            break;
        prev = cur;
    }
    if (prev) {
        lock->nextSortedOutput = prev->nextSortedOutput;
        prev->nextSortedOutput = lock;
    } else {
        lock->nextSortedOutput = sortedOutputLocks;
        sortedOutputLocks = lock;
    }
    return true;
}

bool cls0805c244::addInputLock(cls0805bcc4 *inputLock)
{
    cls0805bcc4 *out = inputLock->parentOrChildren;
    out->activeReferenceCount--;
    inputLock->nextSibling = out->parentOrChildren;
    out->parentOrChildren = inputLock;
    if (!out->calcStats(0)) {
        reportError("AddInputLock failed to calcStats\n");
        exitAfterCleanup(-1);
    }
    inputLock->insertedIntoLockList = 1;
    return true;
}

bool cls0805c244::undoInputLock(cls0805bcc4 *inputLock)
{
    cls0805bcc4 *out = inputLock->parentOrChildren;
    out->activeReferenceCount++;
    out->parentOrChildren = inputLock->nextSibling;
    inputLock->nextSibling = 0;
    if (!out->calcStats(0)) {
        reportError("UndoInputLock failed to calcStats\n");
        exitAfterCleanup(-1);
        return false;
    }
    inputLock->insertedIntoLockList = 0;
    inputLock->resetStats();
    return true;
}

bool cls0805c244::undoOutputLock(cls0805bcc4 *lock)
{
    remainingLockCount++;
    outputLocks = lock->nextSibling;
    lock->nextSibling = 0;
    if (!lock->calcStats(0)) {
        reportError("UndoOutputLock failed to calcStats\n");
        checkCounts();
        exitAfterCleanup(-1);
        return false;
    }
    lock->insertedIntoLockList = 0;
    cls0805bcc4 *prev = 0;
    cls0805bcc4 *cur = sortedOutputLocks;
    while (cur && cur != lock) {
        prev = cur;
        cur = cur->nextSortedOutput;
    }
    if (!cur) {
        reportError("couldn't find lock on the sorted list\n");
        exitAfterCleanup(-1);
    }
    if (prev)
        prev->nextSortedOutput = lock->nextSortedOutput;
    else
        sortedOutputLocks = lock->nextSortedOutput;
    lock->nextSortedOutput = 0;
    lock->resetStats();
    return true;
}

int cls0805c244::maxConstraintOfFirstOutputs(int maximumLockCount)
{
    int best = 0;
    for (cls0805bcc4 *lock = sortedOutputLocks; lock; lock = lock->nextSortedOutput) {
        maximumLockCount--;
        if (lock->latestConsumerCycle > best)
            best = lock->latestConsumerCycle;
        if (lock->activeReferenceCount != 0)
            return 30000;
        if (maximumLockCount <= 0)
            break;
    }
    return best;
}

static inline Operand *lockOperand(cls0805bcc4 *lock)
{
    if (lock->ownerType == 1) {
        int opnd = lock->ownerOperandIndex;
        Instr *instr = (Instr *)lock->ownerPointer;
        return &instr->operands[opnd];
    }
    return 0;
}

static inline bool isInserted(cls0805bcc4 *lock)
{
    return lock->insertedIntoLockList != 0;
}

bool cls0805bcc4::isOk(int type, bool inserted)
{
    int loopSize = activeLoopSize;
    bool lockIsValid = true;
    regUse *registerUse = ownerType == 2 ? (regUse *)ownerPointer : 0;
    Operand *operand = lockOperand(this);

    if (registerUse) {
        if (operand) {
            reportError("rLock::isOk() - both reg ref and op ok\n");
            lockIsValid = false;
        }
    } else if (!operand) {
        reportError("rLock::isOk() - neither op nor regref set\n");
        lockIsValid = false;
    }
    if (type >= 0 && lockDirection != type) {
        reportError("isOk - unexpected type for is ok\n");
        lockIsValid = false;
    }
    if (isInserted(this) != inserted) {
        reportError("isOK - insert state was unexpected!\n");
        lockIsValid = false;
    }
    switch (lockDirection) {
    case 1:
        if (!getParent()) {
            reportError("input lock has no parent\n");
            lockIsValid = false;
        }
        break;
    case 2: {
        if (!inserted && referenceCount != activeReferenceCount) {
            reportError("output lock - ref count mismatch\n");
            return false;
        }
        regUse *orf = ownerType == 2 ? (regUse *)ownerPointer : 0;
        if (orf && loopSize) {
            if (issueLatency > 1) {
                reportError("external input had latency for loop\n");
                lockIsValid = false;
            }
            if (throughputSpacing > 1) {
                reportError("external input had thruput for loop\n");
                lockIsValid = false;
            }
        }
        if (loopSize) {
            if (issueLatency > loopSize && latencyModeFlag && referenceCount) {
                reportError("latency bigger than loop!\n");
                lockIsValid = false;
            }
            if (throughputSpacing > loopSize) {
                reportError("thruput bigger than loop!\n");
                lockIsValid = false;
            }
        }
        break;
    }
    default:
        reportError("rLock::isOk() - bad lock type\n");
        lockIsValid = false;
        break;
    }
    if (!lockIsValid) {
        reportError("rLock::isOk - failed\n");
        print();
        exitAfterCleanup(-1);
    }
    return lockIsValid;
}

static inline bool ownsLock(cls0805c244 *list, cls0805bcc4 *lock)
{
    if (lock->ownerLockList == list)
        return true;
    return false;
}

bool cls0805c244::isOk(cls0805bcc4 *candidateLock, int type, bool inserted)
{
    if (!candidateLock) {
        reportError("rLockList::isOk  -passed a null lock\n");
        exitAfterCleanup(-1);
    }
    bool lockIsValid = candidateLock->isOk(type, inserted);
    if (candidateLock->lockDirection == 0) {
        cls0805bcc4 *parent = candidateLock->getParent();
        cls0805bcc4 *outputLock;
        for (outputLock = outputLocks; outputLock; outputLock = outputLock->nextSibling)
            if (outputLock == parent)
                break;
        if (!outputLock) {
            reportError("rLockList::isOk - parent is not on insert list\n");
            lockIsValid = false;
        }
    }
    if (!ownsLock(this, candidateLock)) {
        reportError("isok - I don't own it\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (candidateLock->lockDirection == 2) {
        if (outputLockCount <= 0) {
            reportError("isOk - output, but no outputs! - <=0 output locks!\n");
            lockIsValid = false;
        }
        int lim = (inserted == false) - 1;
        if (remainingLockCount <= lim) {
            reportError("isOk - output, but no outputs! - <=0 output locks to go!\n");
            lockIsValid = false;
        }
    }
    if (!lockIsValid)
        exitAfterCleanup(-1);
    return lockIsValid;
}

short cls0805bcc4::maximumLockConstraint()
{
    return latestConsumerCycle;
}

short cls0805bcc4::secondaryLockConstraint()
{
    return earliestConsumerFreeCycle;
}

static inline bool hasPartFlag(cls0805bcc4 *lock)
{
    return lock->externalOutputSlotPlusOne && lock->ownerLockList->containsRegisterOutput;
}

bool cls0805bcc4::calcStats(Instr *skip)
{
    int lsz = activeLoopSize;
    cls0805c244 *lockList = ownerLockList;
    scheduledConsumerCount = 0;
    int maxT = -30000;
    int minFree = 30000;
    int minUse = 30000;
    for (cls0805bcc4 *lock = parentOrChildren; lock; lock = lock->nextSibling) {
        if (skip) {
            Instr *otherInstruction = lock->ownerType == 1 ? (Instr *)lock->ownerPointer : 0;
            if (otherInstruction == skip)
                continue;
        }
        if (lock->operandStatusCode == 2)
            continue;
        int consumerRow = lock->linearRow;
        if (consumerRow > maxT)
            maxT = consumerRow;
        if (consumerRow < minUse)
            minUse = consumerRow;
        consumerRow -= lock->issueLatency;
        if (consumerRow < minFree)
            minFree = consumerRow;
        scheduledConsumerCount++;
    }
    if (lockList->calculateLoopStats && lsz) {
        minFree = 0;
        maxT = lsz - 1;
        scheduledConsumerCount = 0;
    }
    if (scheduledConsumerCount != 0) {
        earliestConsumerFreeCycle = minFree;
        latestConsumerCycle = maxT;
        if (lsz) {
            if (isRegLock(this)) {
                earliestReadyCycle = minFree;
                fullLatencyReadyCycle = minFree;
            } else {
                if (latencyModeFlag)
                    earliestReadyCycle = linearRow + 1;
                else
                    earliestReadyCycle = linearRow + issueLatency;
                fullLatencyReadyCycle = linearRow + issueLatency;
            }
        } else if (isRegLock(this)) {
            if (issueLatency == 0) {
                earliestReadyCycle = 0;
                fullLatencyReadyCycle = 0;
            } else {
                if (latencyModeFlag)
                    earliestReadyCycle = 0;
                else
                    earliestReadyCycle = linearRow + issueLatency;
                fullLatencyReadyCycle = linearRow + issueLatency;
            }
        } else {
            if (latencyModeFlag)
                earliestReadyCycle = linearRow + 1;
            else
                earliestReadyCycle = linearRow + issueLatency;
            fullLatencyReadyCycle = linearRow + issueLatency;
        }
        if (earliestConsumerFreeCycle < fullLatencyReadyCycle) {
            if (fatalReadBeforeReady) {
                reportError("rLock::calcStats - rfUsed before result ready..rfu=%d rr=%d!\n", earliestConsumerFreeCycle, fullLatencyReadyCycle);
                print();
                return false;
            }
            if (minUse < earliestReadyCycle)
                reportWarning("rLock::calcStats - stalls non fatal, but.. rfUsed_nolh before result write..rfu=%d rw=%d..this maybe important\n", minUse, earliestReadyCycle);
        }
    } else {
        if (isRegLock(this)) {
            if (issueLatency == 0) {
                earliestReadyCycle = 0;
                fullLatencyReadyCycle = 0;
            } else {
                if (latencyModeFlag)
                    earliestReadyCycle = 0;
                else
                    earliestReadyCycle = linearRow + issueLatency;
                fullLatencyReadyCycle = linearRow + issueLatency;
            }
        } else {
            if (latencyModeFlag)
                earliestReadyCycle = linearRow + 1;
            else
                earliestReadyCycle = linearRow + issueLatency;
            fullLatencyReadyCycle = linearRow + issueLatency;
        }
        earliestConsumerFreeCycle = earliestReadyCycle;
        latestConsumerCycle = earliestReadyCycle;
    }
    int shift = loopRow - linearRow;
    adjustedEarliestReadyCycle = shift + earliestReadyCycle;
    adjustedFullLatencyReadyCycle = shift + fullLatencyReadyCycle;
    adjustedEarliestConsumerFreeCycle = shift + earliestConsumerFreeCycle;
    adjustedLatestConsumerCycle = shift + latestConsumerCycle;
    if (referenceCount != 0 || hasPartFlag(this)) {
        latestReadyCycle = latestConsumerCycle;
        if (!lsz || ownerType != 2) {
            int maxConsumerCount, minConsumerCount;
            int upperConsumerCount = upperPipeConsumerCount, lowerConsumerCount = lowerPipeConsumerCount;
            int len = eitherPipeConsumerCount;
            maxConsumerCount = upperConsumerCount; minConsumerCount = lowerConsumerCount;
            if (upperConsumerCount < lowerConsumerCount) { maxConsumerCount = lowerConsumerCount; minConsumerCount = upperConsumerCount; }
            int consumerCountDifference = maxConsumerCount - minConsumerCount;
            len -= consumerCountDifference;
            if (len > 0)
                maxConsumerCount += (len + 1) >> 1;
            if (maxConsumerCount > 0)
                maxConsumerCount--;
            maxConsumerCount += fullLatencyReadyCycle;
            if (maxConsumerCount > latestConsumerCycle)
                latestReadyCycle = maxConsumerCount;
        }
    } else
        latestReadyCycle = earliestReadyCycle;
    adjustedLatestReadyCycle = shift + latestReadyCycle;
    return true;
}


void cls0805bcc4::resetCalculatedStats()
{
    resetStats();
}

void cls0805bcc4::setFrom(InstructionPosition *instructionPosition)
{
    int rline = instructionPosition->linearRow;
    linearRow = rline;
    int loop = instructionPosition->loopRow;
    loopRow = loop;
    int lsz = activeLoopSize;
    if (lsz) {
        if (linearRow % lsz == loopRow)
            return;
        reportError("rLock::setFrom: rline/loop mismatch %d vs %d (lsz%d)\n", linearRow, loopRow, lsz);
    } else {
        if (linearRow == loopRow)
            return;
        reportError("setFrom rline/non-loop mismatch\n");
    }
    exitAfterCleanup(-1);
}

short cls0805bcc4::getCurrentLoopIteration()
{
    if (activeLoopSize)
        return linearRow / activeLoopSize;
    return 0;
}

bool cls0805c244::moveOutputLock(cls0805bcc4 *lock)
{
    if (!lock) {
        reportError("moveoutputlock - null pointer\n");
        return false;
    }
    if (lock->lockDirection != 2) {
        reportError("moveoutputlock - bad type\n");
        return false;
    }
    if (!lock->insertedIntoLockList) {
        reportError("moveoutputlock - not already inserted\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (!lock->calcStats(0)) {
        reportError("moveoutputlock failed to set lUsed\n");
        checkCounts();
        exitAfterCleanup(-1);
    }
    cls0805bcc4 *cur = sortedOutputLocks;
    cls0805bcc4 *prev = 0;
    sortedOutputLocks = 0;
    while (cur) {
        cls0805bcc4 *next = cur->nextSortedOutput;
        cur->nextSortedOutput = 0;
        if (cur != lock) {
            if (!sortedOutputLocks)
                sortedOutputLocks = cur;
            else
                prev->nextSortedOutput = cur;
            prev = cur;
        }
        cur = next;
    }
    prev = 0;
    for (cur = sortedOutputLocks; cur; cur = cur->nextSortedOutput) {
        int lockLoopRow = lock->loopRow;
        int crl = cur->loopRow;
        if ((short)lockLoopRow == (short)crl) {
            reportError("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            exitAfterCleanup(-1);
        }
        int line = lock->linearRow;
        int cline = cur->linearRow;
        if ((short)line < (short)cline)
            break;
        prev = cur;
    }
    if (prev) {
        lock->nextSortedOutput = prev->nextSortedOutput;
        prev->nextSortedOutput = lock;
    } else {
        lock->nextSortedOutput = sortedOutputLocks;
        sortedOutputLocks = lock;
    }
    return true;
}

bool cls0805c244::moveInputLock(cls0805bcc4 *inputLock)
{
    if (!inputLock->parentOrChildren->calcStats(0)) {
        reportError("moveinputlock failed to calcStats\n");
        exitAfterCleanup(-1);
    }
    return true;
}

static inline bool singleExtOutputMode()
{
    if (fatalReadBeforeReady == 1)
        return true;
    return false;
}

bool cls0805c244::addExtOutput(cls0805bcc4 *lock, bool flag)
{
    if (lock->externalOutputSlotPlusOne) {
        reportError("lock is already external output\n");
        return false;
    }
    if (externalOutputCount > 3) {
        reportError("too many extern outputs\n");
        return false;
    }
    if (externalOutputCount && singleExtOutputMode()) {
        if (flag)
            return true;
        reportError("more than one xo for rlock uta..count=%d\n", externalOutputCount);
        checkCounts();
        return false;
    }
    sharedExternalOutputFlag = flag;
    externalOutputLocks[externalOutputCount] = lock;
    lock->externalOutputSlotPlusOne = ++externalOutputCount;
    return true;
}

static inline bool hasRegUseOwner(cls0805bcc4 *lock)
{
    if (lock->ownerType == 2)
        return true;
    return false;
}

Instr *cls0805c244::earliestExt()
{
    Instr *res = 0;
    cls0805bcc4 *lock = sortedOutputLocks;
    if (lock) {
        if (hasRegUseOwner(lock)) {
            int best = 30000;
            for (cls0805bcc4 *childLock = lock->parentOrChildren; childLock; childLock = childLock->nextSibling) {
                if (childLock->operandStatusCode != 2) {
                    Instr *instruction = childLock->ownerType == 1 ? (Instr *)childLock->ownerPointer : 0;
                    if (!instruction) {
                        reportError("not get in for earliest ext\n");
                        exitAfterCleanup(-1);
                    }
                    if (instruction->position.linearRow < best) {
                        best = instruction->insertionSerial;
                        res = instruction;
                    }
                }
            }
        }
    }
    return res;
}

int cls0805c244::countFlaggedOutputLocks()
{
    cls0805bcc4 *lock = outputLocks;
    int activeLockCount = 0;
    while (lock) {
        if (lock->activeReferenceCount)
            activeLockCount++;
        lock = lock->nextSibling;
    }
    return activeLockCount;
}

bool freeLockLists(cls0805c244 *firstLockList)
{
    while (firstLockList) {
        cls0805c244 *next = firstLockList->nextLockList;
        if (firstLockList)
            ::operator delete(firstLockList);
        firstLockList = next;
    }
    return true;
}

cls0805c244 *findLockList(cls0805c244 *firstLockList, RegPoolEntry *reg, int part)
{
    for (; firstLockList; firstLockList = firstLockList->nextLockList) {
        if (firstLockList->registerEntry == reg && firstLockList->componentIndex == part) {
            if (reg->getPartPayload(part) && reg->getPartPayload(part) != (VclPtrWord)firstLockList) {
                reportError("rg points to rll, but rll list not it\n");
                exitAfterCleanup(-1);
            }
            return firstLockList;
        }
    }
    return 0;
}

const char *rlmtName(unsigned char lockType)
{
    switch (lockType) {
    case 1:
        return "RLMT_MOVEAGE";
    case 2:
        return "RLMT_AUTOAGE";
    case 0:
        return "RLMT_NOMOVE";
    default:
        return "RLMT_UNKNOWN";
    }
}

const char *cls0805bcc4::regName()
{
    cls0805c244 *lockList = ownerLockList;
    if (!lockList)
        return "NOT_ON_A_LOCKLIST";
    RegPoolEntry *registerEntry = lockList->registerEntry;
    if (!registerEntry)
        return "RLL_HAS_NO_REG";
    return registerEntry->registerName();
}

bool cls0805c244::setRepReg(RegPoolEntry *selectedRegister)
{
    if (registerEntry && selectedRegister != registerEntry) {
        reportError("can't set representative reg - it's already set\n");
        return false;
    }
    registerEntry = selectedRegister;
    return true;
}

static inline void wrapRange(int &rangeStart, int &rangeEnd)
{
    if (activeLoopSize) {
        while (rangeStart >= activeLoopSize) {
            rangeStart -= activeLoopSize;
            rangeEnd -= activeLoopSize;
        }
    }
}

static inline void wrapRangeLocal(int &rangeStart, int &rangeEnd)
{
    int loopPeriod = activeLoopSize;
    if (loopPeriod) {
        while (rangeStart >= loopPeriod) {
            rangeStart -= loopPeriod;
            rangeEnd -= loopPeriod;
        }
    }
}
