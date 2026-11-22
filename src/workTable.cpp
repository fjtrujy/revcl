extern const char thruSumOobError[] = "thruSum was set - but - oBits wasn't set for a reg part..register %s, part %d\n";
#define VCL_REGUSESLOT_IMPL
#include "RegPool.h"
#include "ol_s.h"
#include "os.h"
#include "workTable.h"

RegPool *GetRP(unsigned t);

regUseSlot::~regUseSlot()
{
}

void regUseList::dumpRegisterUses()
{
    if (hasConsistentCount()) {
        for (int i = 0; i != 12; i++) {
            RegPool *pool = GetRP(i);
            if (pool)
                dumpRegisterClassUses(i);
        }
    }
}

extern const char regUseNewFailedError[] = "AddNewRegRef() - new failed\n";

int regUseList::dumpRegisterClassUses(int regClass)
{
    int n = 0;
    regUse *listHead = head;
    regUse *registerUse;
    for (registerUse = listHead; registerUse; registerUse = registerUse->next) {
        RegPoolEntry *reg = registerUse->registerEntry;
        if (!reg) {
            reportError("found reg ref with no reg!\n");
            exitAfterCleanup(-1);
            return 0;
        }
        if (reg->registerClass == regClass)
            n++;
    }
    if (n == 0)
        return 0;
    for (registerUse = listHead; registerUse; registerUse = registerUse->next) {
        if (registerUse->registerEntry->isClass(regClass))
            registerUse->validateAndDumpParts();
    }
    return n;
}

void regUse::validateAndDumpParts()
{
    RegPoolEntry *reg = registerEntry;
    int i;
    int j = 0;
    if (reg->partCount != 0)
        do
            j++;
        while (j != reg->partCount);
    for (i = 0; i != reg->partCount; i++) {
        if (slots[i].throughputSums[0] && !((outputMask >> i) & 1)) {
            reportError(thruSumOobError,
                          reg->registerName(), i);
            exitAfterCleanup(-1);
        }
    }
    for (i = 0; i != reg->partCount; i++) {
        if (slots[i].throughputPairs.count)
            slots[i].throughputPairs.dump();
    }
}

regUse *regUseList::findByRegister(RegPoolEntry *reg)
{
    return find(reg);
}

regUse *regUseList::firstReal()
{
    regUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        if (!reg) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (!reg->isVirtual)
            break;
    }
    return previousUse;
}

regUse *regUse::nextReal()
{
    regUse *previousUse;
    for (previousUse = next; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        if (!reg) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (!reg->isVirtual)
            break;
    }
    return previousUse;
}

regUse *regUseList::firstVirtual()
{
    regUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        if (!reg) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (reg->isVirtual)
            break;
    }
    return previousUse;
}

regUse *regUse::nextVirtual()
{
    regUse *previousUse;
    for (previousUse = next; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        if (!reg) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (reg->isVirtual)
            break;
    }
    return previousUse;
}

bool regUseList::optReset()
{
    for (regUse *p = head; p; p = p->next) {
        for (int i = 0; i != 4; i++) {
            p->slots[i].assignmentPayload = 0;
            p->slots[i].registerLock.resetLock();
        }
    }
    return true;
}

bool regUse::optReset()
{
    for (int i = 0; i != 4; i++) {
        slots[i].assignmentPayload = 0;
        slots[i].registerLock.resetLock();
    }
    return true;
}

bool regUseSlot::optReset()
{
    assignmentPayload = 0;
    registerLock.resetLock();
    return true;
}

bool regUseSlot::addOutput(ol_s *operandLocation)
{
    if (!outputLocations.insertExternalLocation(operandLocation)) {
        reportError("failed to add oploc to output list sort\n");
        return false;
    }
    return true;
}

void regUseSlot::validateOutputLocations()
{
    outputLocations.validateOperandLocations();
}

void regUseSlot::validateInputLocations()
{
    inputLocations.validateOperandLocations();
}

OperandLock *regUseSlot::getRegisterLock()
{
    return &registerLock;
}

regUse *regUse::Clone()
{
    RegPoolEntry *reg = registerEntry;
    if (!reg) {
        reportError("regref::Clone - input had no register\n");
        return 0;
    }
    regUse *n = new regUse(reg);
    if (!n) {
        reportError("failed to get new regref in clone\n");
        return 0;
    }
    n->unk_18 = unk_18;
    n->hazardMask = hazardMask;
    n->definingInstruction = definingInstruction;
    n->sourcePathHazardMask = sourcePathHazardMask;
    n->hazardBroadcastMask = hazardBroadcastMask;
    n->externalInputMask = externalInputMask;
    n->registerEntry = registerEntry;
    n->externalOutputMask = externalOutputMask;
    n->inputMask = inputMask;
    n->outputMask = outputMask;
    n->throughputMask = throughputMask;
    n->unk_16 = unk_16;
    n->unk_11 = unk_11;
    n->externalBlockMode = externalBlockMode;
    for (int i = 0; i != reg->partCount; i++) {
        n->slots[i].throughputSums[0] = slots[i].throughputSums[0];
        n->slots[i].throughputSums[1] = slots[i].throughputSums[1];
        n->slots[i].throughputSums[3] = slots[i].throughputSums[3];
        n->slots[i].throughputSums[2] = slots[i].throughputSums[2];
        n->slots[i].timingValues[0] = slots[i].timingValues[0];
        n->slots[i].timingValues[1] = slots[i].timingValues[1];
        n->slots[i].timingValues[2] = slots[i].timingValues[2];
        n->slots[i].timingValues[3] = slots[i].timingValues[3];
        n->slots[i].timingValues[4] = slots[i].timingValues[4];
        n->slots[i].timingValues[5] = slots[i].timingValues[5];
        n->slots[i].timingValues[6] = slots[i].timingValues[6];
        n->slots[i].timingValues[7] = slots[i].timingValues[7];
        n->slots[i].timingValues[8] = slots[i].timingValues[8];
        n->slots[i].timingValues[9] = slots[i].timingValues[9];
        n->slots[i].timingValues[10] = slots[i].timingValues[10];
        n->slots[i].timingValues[11] = slots[i].timingValues[11];
        n->slots[i].timingValues[12] = slots[i].timingValues[12];
        n->slots[i].earliestExternalCycle = slots[i].earliestExternalCycle;
        n->slots[i].latestExternalCycle = slots[i].latestExternalCycle;
        n->slots[i].assignmentPayload = 0;
        n->slots[i].registerLock.resetLock();
    }
    return n;
}

const char *regUse::regName()
{
    if (!registerEntry)
        return "REGREG_NOREG";
    return registerEntry->registerName();
}

ol_s *regUseSlot::findPreviousOutputAtPosition(int pos, bool all)
{
    if (all)
        pos = 0xff;
    ol_s *prev = 0;
    for (ol_s *currentLocation = outputLocations.head; currentLocation; currentLocation = currentLocation->next) {
        if (pos < currentLocation->cycleOffset)
            break;
        prev = currentLocation;
    }
    return prev;
}

int regUseList::countUncoveredRegisterClassUses(int regClass)
{
    int n = 0;
    for (regUse *previousUse = head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        if (reg->registerClass != regClass || reg == reg->ownerPool->zeroRegister)
            continue;
        if (~previousUse->inputMask & previousUse->throughputMask)
            n++;
        else if (!previousUse->outputMask)
            n++;
    }
    return n;
}

void regUseList::dumpVirtualRegisterUses()
{
    if (hasConsistentCount()) {
        for (int i = 0; i != 12; i++) {
            RegPool *pool = GetRP(i);
            if (pool && pool->virtualPartCount) {
                reportInfo("%s\n", pool->poolName);
                dumpRegisterClassAllocations(i);
            }
        }
    }
}

static inline bool isVirtualRegister(RegPoolEntry *reg)
{
    if (reg->isVirtual == true)
        return true;
    return false;
}

void regUseList::dumpRegisterClassAllocations(int regClass)
{
    regUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        RegPoolEntry *phys = reg->assignedPhysicalRegister;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->registerName();
        if (!isVirtualRegister(reg))
            alloc = reg->registerName();
        int mask = previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask | previousUse->throughputMask;
        if (reg->registerClass == regClass && reg != reg->ownerPool->zeroRegister && previousUse->isExternal()) {
            const char *thr = "";
            if (previousUse->throughputMask & ~(previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask))
                thr = "-THR";
            reportInfo(" EXTERNAL: %20s - %s %s\n", reg->maskName(mask), alloc, thr);
        }
    }
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        RegPoolEntry *phys = reg->assignedPhysicalRegister;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->registerName();
        if (!isVirtualRegister(reg))
            alloc = reg->registerName();
        int mask = previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask | previousUse->throughputMask;
        if (reg->registerClass == regClass && reg != reg->ownerPool->zeroRegister && !previousUse->isExternal())
            reportInfo(" LOCAL : %20s - %s\n", reg->maskName(mask), alloc);
    }
}
