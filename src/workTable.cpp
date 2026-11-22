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
    int partIndex;
    int j = 0;
    if (reg->partCount != 0)
        do
            j++;
        while (j != reg->partCount);
    for (partIndex = 0; partIndex != reg->partCount; partIndex++) {
        if (slots[partIndex].throughputSums[0] && !((outputMask >> partIndex) & 1)) {
            reportError(thruSumOobError,
                          reg->registerName(), partIndex);
            exitAfterCleanup(-1);
        }
    }
    for (partIndex = 0; partIndex != reg->partCount; partIndex++) {
        if (slots[partIndex].throughputPairs.count)
            slots[partIndex].throughputPairs.dump();
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
    for (regUse *registerUse = head; registerUse; registerUse = registerUse->next) {
        for (int partIndex = 0; partIndex != 4; partIndex++) {
            registerUse->slots[partIndex].assignmentPayload = 0;
            registerUse->slots[partIndex].registerLock.resetLock();
        }
    }
    return true;
}

bool regUse::optReset()
{
    for (int partIndex = 0; partIndex != 4; partIndex++) {
        slots[partIndex].assignmentPayload = 0;
        slots[partIndex].registerLock.resetLock();
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
    regUse *registerUse = new regUse(reg);
    if (!registerUse) {
        reportError("failed to get new regref in clone\n");
        return 0;
    }
    registerUse->unk_18 = unk_18;
    registerUse->hazardMask = hazardMask;
    registerUse->definingInstruction = definingInstruction;
    registerUse->sourcePathHazardMask = sourcePathHazardMask;
    registerUse->hazardBroadcastMask = hazardBroadcastMask;
    registerUse->externalInputMask = externalInputMask;
    registerUse->registerEntry = registerEntry;
    registerUse->externalOutputMask = externalOutputMask;
    registerUse->inputMask = inputMask;
    registerUse->outputMask = outputMask;
    registerUse->throughputMask = throughputMask;
    registerUse->unk_16 = unk_16;
    registerUse->unk_11 = unk_11;
    registerUse->externalBlockMode = externalBlockMode;
    for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
        registerUse->slots[partIndex].throughputSums[0] = slots[partIndex].throughputSums[0];
        registerUse->slots[partIndex].throughputSums[1] = slots[partIndex].throughputSums[1];
        registerUse->slots[partIndex].throughputSums[3] = slots[partIndex].throughputSums[3];
        registerUse->slots[partIndex].throughputSums[2] = slots[partIndex].throughputSums[2];
        registerUse->slots[partIndex].timingValues[0] = slots[partIndex].timingValues[0];
        registerUse->slots[partIndex].timingValues[1] = slots[partIndex].timingValues[1];
        registerUse->slots[partIndex].timingValues[2] = slots[partIndex].timingValues[2];
        registerUse->slots[partIndex].timingValues[3] = slots[partIndex].timingValues[3];
        registerUse->slots[partIndex].timingValues[4] = slots[partIndex].timingValues[4];
        registerUse->slots[partIndex].timingValues[5] = slots[partIndex].timingValues[5];
        registerUse->slots[partIndex].timingValues[6] = slots[partIndex].timingValues[6];
        registerUse->slots[partIndex].timingValues[7] = slots[partIndex].timingValues[7];
        registerUse->slots[partIndex].timingValues[8] = slots[partIndex].timingValues[8];
        registerUse->slots[partIndex].timingValues[9] = slots[partIndex].timingValues[9];
        registerUse->slots[partIndex].timingValues[10] = slots[partIndex].timingValues[10];
        registerUse->slots[partIndex].timingValues[11] = slots[partIndex].timingValues[11];
        registerUse->slots[partIndex].timingValues[12] = slots[partIndex].timingValues[12];
        registerUse->slots[partIndex].earliestExternalCycle = slots[partIndex].earliestExternalCycle;
        registerUse->slots[partIndex].latestExternalCycle = slots[partIndex].latestExternalCycle;
        registerUse->slots[partIndex].assignmentPayload = 0;
        registerUse->slots[partIndex].registerLock.resetLock();
    }
    return registerUse;
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
