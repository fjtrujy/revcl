#include "os.h"
#include "RegPool_shared.h"

bool RegisterPartState::flushPending()
{
    if (issueLock) {
        reportError("rPart::flushPending - issueLock was set (%d)\n", issueLock);
        return false;
    }
    for (int locationIndex = 0; locationIndex != 4; locationIndex++)
        pendingLocations[locationIndex].reset();
    pendingSource = 0;
    currentLocation.reset();
    return true;
}

OperandLocation* RegisterPartState::acquireFreePendingLocation()
{
    for (int locationIndex = 0; locationIndex != 4; locationIndex++) {
        if (!pendingLocations[locationIndex].cycleOffset) {
            pendingLocations[locationIndex].reset();
            return &pendingLocations[locationIndex];
        }
    }
    return 0;
}

bool RegisterPartState::cycle(bool flag)
{
    if (flag) {
        issueLock = 0;
        if (pendingSource && pendingSource != &currentLocation) {
            currentLocation.copyLocationFrom(pendingSource);
            pendingSource = &currentLocation;
        }
        for (int locationIndex = 0; locationIndex != 4; locationIndex++)
            pendingLocations[locationIndex].reset();
        OperandLocation* cur = &currentLocation;
        if (cur->cycleOffset)
            cur->cycleOffset = 0;
    } else {
        int expiredLocationCount = 0;
        OperandLocation* cur = &currentLocation;
        int currentCycleOffset = cur->cycleOffset;
        if (currentCycleOffset) {
            int nextCycleOffset = currentCycleOffset - 1;
            cur->cycleOffset = nextCycleOffset > 255 ? 255 : nextCycleOffset;
            expiredLocationCount = 1;
        }
        for (int locationIndex = 0; locationIndex != 4; locationIndex++) {
            OperandLocation* location = &pendingLocations[locationIndex];
            int pendingCycleOffset = location->cycleOffset;
            if (!pendingCycleOffset)
                continue;
            int decrementedCycleOffset = pendingCycleOffset - 1;
            location->cycleOffset = decrementedCycleOffset > 255 ? 255 : decrementedCycleOffset;
            if (decrementedCycleOffset == 0) {
                if (++expiredLocationCount > 1) {
                    reportError("WARNING !! CYCLE wrote thru more than once!\n");
                    return false;
                }
                cur->copyLocationFrom(location);
                cycleAge = 0;
            }
        }
    }
    if (++cycleAge == 0x7fff)
        cycleAge = 0x7fff;
    if (issueLock)
        issueLock--;
    return true;
}

void RegisterEntry::resetRegisterFlowState()
{
    usedSlotMask = 0;
    throughputWriter = 0;
    outputWriteCount = 0;
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].resetPartFlowState();
}

void RegisterEntry::noopRegisterFlowHook()
{
}

void RegisterEntry::resetRegisterPartTiming()
{
    for (int partIndex = 0; partIndex != 4; partIndex++) {
        RegisterPartState* registerPart = &parts[partIndex];
        registerPart->resetOnlyRegisterPartTimingSentinel = -1;
        registerPart->earliestPendingReadCycle = -1;
        registerPart->latestPendingReadCycle = -1;
    }
}

void RegisterPartState::resetPartTiming()
{
    resetOnlyRegisterPartTimingSentinel = -1;
    earliestPendingReadCycle = -1;
    latestPendingReadCycle = -1;
}

void RegisterEntry::resetRegisterThroughputState()
{
    throughputWriter = 0;
    outputWriteCount = 0;
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].resetPartFlowState();
}

void RegisterPartState::resetPartFlowState()
{
    emulationTimingMetrics[3] = 0;
    emulationTimingMetrics[2] = 0;
    emulationTimingMetrics[0] = 0;
    emulationTimingMetrics[1] = 0;
    issueLock = 0;
    currentLocation.reset();
    pendingSource = 0;
    cycleAge = 0;
    emulationTimingMetrics[7] = -1;
    emulationTimingMetrics[4] = -1;
    emulationTimingMetrics[8] = -1;
    emulationTimingMetrics[5] = -1;
    emulationTimingMetrics[6] = -1;
    emulationTimingMetrics[12] = -1;
    emulationTimingMetrics[9] = -1;
    emulationTimingMetrics[13] = -1;
    emulationTimingMetrics[10] = -1;
    emulationTimingMetrics[14] = -1;
    emulationTimingMetrics[11] = -1;
    emulationTimingMetrics[16] = -1;
    emulationTimingMetrics[15] = -1;
    for (int locationIndex = 0; locationIndex != 4; locationIndex++)
        pendingLocations[locationIndex].reset();
}

void RegisterEntry::clearRegisterPartPayloads()
{
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].payload = 0;
}

void RegisterPartState::resetPartPayloadFirstPass()
{
    payload = 0;
}

void RegisterPartState::resetPartPayloadSecondPass()
{
    payload = 0;
}

bool RegisterEntry::cycleRegisterParts(bool flag)
{
    for (int partIndex = 0; partIndex != partCount; partIndex++)
        if (!parts[partIndex].cycle(flag))
            return false;
    return true;
}

RegisterEntry::RegisterEntry()
{
    previous = 0;
    next = 0;
    registerClass = 0;
    partCount = 0;
    allocatable = false;
    moveInstruction = 0;
    registerIndex = 0;
    isVirtual = false;
    ownerPool = 0;
    registerWeb = 0;
    uniqueNameSerial = 0;
    enabledPhysicalRegister = false;
    usedSlotMask = 0;
    nonBlockId = 0;
    tracksHotOutputPaths = false;
    assignedPhysicalRegister = 0;
}

RegisterPartState::RegisterPartState()
{
    resetPartFlowState();
    payload = 0;
}

mNode* RegisterEntry::addRegisterNameAlias(const char* name)
{
    mNode* macroNode = nameAliases.findByExactName(name);
    if (macroNode)
        return macroNode;
    return nameAliases.appendNamedNode(name);
}

const char* RegisterEntry::registerName()
{
    const char* name = nameAliases.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    return name;
}

VclPtrWord RegisterEntry::getPartPayload(int partIndex)
{
    return parts[partIndex].payload;
}

bool RegisterEntry::setPartPayload(int partIndex, VclPtrWord partPayload)
{
    parts[partIndex].payload = partPayload;
    return true;
}

VclPtrWord RegisterPartState::getPayload()
{
    return payload;
}

bool RegisterPartState::setPayload(VclPtrWord newPayload)
{
    payload = newPayload;
    return true;
}

OperandLocation* RegisterPartState::getPendingLocation(int pendingIndex)
{
    return &pendingLocations[pendingIndex];
}

void RegisterEntry::moveReset()
{
    if (!moveInstruction) {
        reportError("rg..movereset called - but not move\n");
        exitAfterCleanup(-1);
    }
    assignmentCostTableSize = -1;
    allocatable = false;
    registerWeb = 0;
    uniqueNameSerial = 0;
    for (int partIndex = 0; partIndex != partCount; partIndex++)
        parts[partIndex].payload = 0;
}

bool RegisterEntry::optReset()
{
    registerWeb = 0;
    return true;
}

bool RegisterEntry::clearPartPayloadMarkers()
{
    for (int partIndex = 0; partIndex != partCount; partIndex++) {
        if (parts[partIndex].payload)
            ((char*)parts[partIndex].payload)[3] = 0;
    }
    return true;
}

char regNameBuf[1024];
int regNameBufPos;

char* RegisterEntry::maskName(int mask)
{
    if (regNameBufPos > 512)
        regNameBufPos = 0;
    char* buf = &regNameBuf[regNameBufPos];
    const char* name = nameAliases.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    char* outputCursor = buf + formatIntoBuffer(buf, "%s", name);
    if (partCount > 1)
        outputCursor += formatIntoBuffer(outputCursor, "%s", formatDottedComponentMask(mask));
    *outputCursor = 0;
    regNameBufPos = outputCursor - regNameBuf + 1;
    return buf;
}

int RegisterEntry::regIndex()
{
    if (!ownerPool)
        reportError("no reg pool set for register\n");
    return ownerPool->regIndex(registerIndex);
}
