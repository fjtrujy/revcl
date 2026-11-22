#include "os.h"
#include "RegPool_shared.h"

bool rPart::flushPending()
{
    if (issueLock) {
        reportError("rPart::flushPending - issueLock was set (%d)\n", issueLock);
        return false;
    }
    for (int i = 0; i != 4; i++)
        pendingLocations[i].reset();
    pendingSource = 0;
    currentLocation.reset();
    return true;
}

ol_s* rPart::acquireFreePendingLocation()
{
    for (int i = 0; i != 4; i++) {
        if (!pendingLocations[i].cycleOffset) {
            pendingLocations[i].reset();
            return &pendingLocations[i];
        }
    }
    return 0;
}

bool rPart::cycle(bool flag)
{
    if (flag) {
        issueLock = 0;
        if (pendingSource && pendingSource != &currentLocation) {
            currentLocation.copyLocationFrom(pendingSource);
            pendingSource = &currentLocation;
        }
        for (int i = 0; i != 4; i++)
            pendingLocations[i].reset();
        ol_s* cur = &currentLocation;
        if (cur->cycleOffset)
            cur->cycleOffset = 0;
    } else {
        int expiredLocationCount = 0;
        ol_s* cur = &currentLocation;
        int currentCycleOffset = cur->cycleOffset;
        if (currentCycleOffset) {
            int nextCycleOffset = currentCycleOffset - 1;
            cur->cycleOffset = nextCycleOffset > 255 ? 255 : nextCycleOffset;
            expiredLocationCount = 1;
        }
        for (int i = 0; i != 4; i++) {
            ol_s* location = &pendingLocations[i];
            int c = location->cycleOffset;
            if (!c)
                continue;
            int v = c - 1;
            location->cycleOffset = v > 255 ? 255 : v;
            if (v == 0) {
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

void RegPoolEntry::resetRegisterFlowState()
{
    usedSlotMask = 0;
    throughputWriter = 0;
    unk_36 = 0;
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].resetPartFlowState();
}

void RegPoolEntry::noopRegisterFlowHook()
{
}

void RegPoolEntry::resetRegisterPartTiming()
{
    for (int partIndex = 0; partIndex != 4; partIndex++) {
        rPart* registerPart = &parts[partIndex];
        registerPart->unk_40 = -1;
        registerPart->earliestPendingReadCycle = -1;
        registerPart->latestPendingReadCycle = -1;
    }
}

void rPart::resetPartTiming()
{
    unk_40 = -1;
    earliestPendingReadCycle = -1;
    latestPendingReadCycle = -1;
}

void RegPoolEntry::resetRegisterThroughputState()
{
    throughputWriter = 0;
    unk_36 = 0;
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].resetPartFlowState();
}

void rPart::resetPartFlowState()
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
    for (int i = 0; i != 4; i++)
        pendingLocations[i].reset();
}

void RegPoolEntry::clearRegisterPartPayloads()
{
    for (int partIndex = 0; partIndex != 4; partIndex++)
        parts[partIndex].payload = 0;
}

void rPart::resetPartPayloadFirstPass()
{
    payload = 0;
}

void rPart::resetPartPayloadSecondPass()
{
    payload = 0;
}

bool RegPoolEntry::cycleRegisterParts(bool flag)
{
    for (int partIndex = 0; partIndex != partCount; partIndex++)
        if (!parts[partIndex].cycle(flag))
            return false;
    return true;
}

RegPoolEntry::RegPoolEntry()
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

rPart::rPart()
{
    resetPartFlowState();
    payload = 0;
}

mNode* RegPoolEntry::addRegisterNameAlias(const char* name)
{
    mNode* macroNode = nameAliases.findByExactName(name);
    if (macroNode)
        return macroNode;
    return nameAliases.appendNamedNode(name);
}

const char* RegPoolEntry::registerName()
{
    const char* name = nameAliases.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    return name;
}

VclPtrWord RegPoolEntry::getPartPayload(int partIndex)
{
    return parts[partIndex].payload;
}

bool RegPoolEntry::setPartPayload(int partIndex, VclPtrWord partPayload)
{
    parts[partIndex].payload = partPayload;
    return true;
}

VclPtrWord rPart::getPayload()
{
    return payload;
}

bool rPart::setPayload(VclPtrWord newPayload)
{
    payload = newPayload;
    return true;
}

ol_s* rPart::getPendingLocation(int pendingIndex)
{
    return &pendingLocations[pendingIndex];
}

void RegPoolEntry::moveReset()
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

bool RegPoolEntry::optReset()
{
    registerWeb = 0;
    return true;
}

bool RegPoolEntry::clearPartPayloadMarkers()
{
    for (int partIndex = 0; partIndex != partCount; partIndex++) {
        if (parts[partIndex].payload)
            ((char*)parts[partIndex].payload)[3] = 0;
    }
    return true;
}

char regNameBuf[1024];
int regNameBufPos;

char* RegPoolEntry::maskName(int mask)
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

int RegPoolEntry::regIndex()
{
    if (!ownerPool)
        reportError("no reg pool set for register\n");
    return ownerPool->regIndex(registerIndex);
}
