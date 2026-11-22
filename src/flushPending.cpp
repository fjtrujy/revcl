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
        int n = 0;
        ol_s* cur = &currentLocation;
        int c = cur->cycleOffset;
        if (c) {
            int v = c - 1;
            cur->cycleOffset = v > 255 ? 255 : v;
            n = 1;
        }
        for (int i = 0; i != 4; i++) {
            ol_s* p = &pendingLocations[i];
            int c = p->cycleOffset;
            if (!c)
                continue;
            int v = c - 1;
            p->cycleOffset = v > 255 ? 255 : v;
            if (v == 0) {
                if (++n > 1) {
                    reportError("WARNING !! CYCLE wrote thru more than once!\n");
                    return false;
                }
                cur->copyLocationFrom(p);
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
    for (int i = 0; i != 4; i++)
        parts[i].resetPartFlowState();
}

void RegPoolEntry::noopRegisterFlowHook()
{
}

void RegPoolEntry::resetRegisterPartTiming()
{
    for (int i = 0; i != 4; i++) {
        rPart* p = &parts[i];
        p->unk_40 = -1;
        p->unk_42 = -1;
        p->unk_44 = -1;
    }
}

void rPart::resetPartTiming()
{
    unk_40 = -1;
    unk_42 = -1;
    unk_44 = -1;
}

void RegPoolEntry::resetRegisterThroughputState()
{
    throughputWriter = 0;
    unk_36 = 0;
    for (int i = 0; i != 4; i++)
        parts[i].resetPartFlowState();
}

void rPart::resetPartFlowState()
{
    unk_00[3] = 0;
    unk_00[2] = 0;
    unk_00[0] = 0;
    unk_00[1] = 0;
    issueLock = 0;
    currentLocation.reset();
    pendingSource = 0;
    cycleAge = 0;
    unk_00[7] = -1;
    unk_00[4] = -1;
    unk_00[8] = -1;
    unk_00[5] = -1;
    unk_00[6] = -1;
    unk_00[12] = -1;
    unk_00[9] = -1;
    unk_00[13] = -1;
    unk_00[10] = -1;
    unk_00[14] = -1;
    unk_00[11] = -1;
    unk_00[16] = -1;
    unk_00[15] = -1;
    for (int i = 0; i != 4; i++)
        pendingLocations[i].reset();
}

void RegPoolEntry::clearRegisterPartPayloads()
{
    for (int i = 0; i != 4; i++)
        parts[i].payload = 0;
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
    for (int i = 0; i != partCount; i++)
        if (!parts[i].cycle(flag))
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
    unk_2c = false;
    assignedPhysicalRegister = 0;
}

rPart::rPart()
{
    resetPartFlowState();
    payload = 0;
}

mNode* RegPoolEntry::addRegisterNameAlias(const char* name)
{
    mNode* n = nameAliases.findByExactName(name);
    if (n)
        return n;
    return nameAliases.appendNamedNode(name);
}

const char* RegPoolEntry::registerName()
{
    const char* name = nameAliases.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    return name;
}

VclPtrWord RegPoolEntry::getPartPayload(int i)
{
    return parts[i].payload;
}

bool RegPoolEntry::setPartPayload(int i, VclPtrWord v)
{
    parts[i].payload = v;
    return true;
}

VclPtrWord rPart::getPayload()
{
    return payload;
}

bool rPart::setPayload(VclPtrWord v)
{
    payload = v;
    return true;
}

ol_s* rPart::getPendingLocation(int i)
{
    return &pendingLocations[i];
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
    for (int i = 0; i != partCount; i++)
        parts[i].payload = 0;
}

bool RegPoolEntry::optReset()
{
    registerWeb = 0;
    return true;
}

bool RegPoolEntry::clearPartPayloadMarkers()
{
    for (int i = 0; i != partCount; i++) {
        if (parts[i].payload)
            ((char*)parts[i].payload)[3] = 0;
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
    char* p = buf + formatIntoBuffer(buf, "%s", name);
    if (partCount > 1)
        p += formatIntoBuffer(p, "%s", formatDottedComponentMask(mask));
    *p = 0;
    regNameBufPos = p - regNameBuf + 1;
    return buf;
}

int RegPoolEntry::regIndex()
{
    if (!ownerPool)
        reportError("no reg pool set for register\n");
    return ownerPool->regIndex(registerIndex);
}
