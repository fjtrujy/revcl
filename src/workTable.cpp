extern const char thruSumOobError[] = "thruSum was set - but - oBits wasn't set for a reg part..register %s, part %d\n";
#define VCL_REGUSESLOT_IMPL
#include "RegPool.h"
#include "ol_s.h"
#include "os.h"
#include "workTable.h"

RegPool *getRegisterPoolByIndex(unsigned t);

regUseSlot::~regUseSlot()
{
}

void RegisterUseList::dumpRegisterUses()
{
    if (hasConsistentCount()) {
        for (int poolIndex = 0; poolIndex != 12; poolIndex++) {
            RegPool *pool = getRegisterPoolByIndex(poolIndex);
            if (pool)
                dumpRegisterClassUses(poolIndex);
        }
    }
}

extern const char regUseNewFailedError[] = "AddNewRegRef() - new failed\n";

int RegisterUseList::dumpRegisterClassUses(int regClass)
{
    int registerUseCount = 0;
    RegisterUse *listHead = head;
    RegisterUse *registerUse;
    for (registerUse = listHead; registerUse; registerUse = registerUse->next) {
        RegisterEntry *registerEntry = registerUse->registerEntry;
        if (!registerEntry) {
            reportError("found reg ref with no reg!\n");
            exitAfterCleanup(-1);
            return 0;
        }
        if (registerEntry->registerClass == regClass)
            registerUseCount++;
    }
    if (registerUseCount == 0)
        return 0;
    for (registerUse = listHead; registerUse; registerUse = registerUse->next) {
        if (registerUse->registerEntry->isClass(regClass))
            registerUse->validateAndDumpParts();
    }
    return registerUseCount;
}

void RegisterUse::validateAndDumpParts()
{
    RegisterEntry *reg = registerEntry;
    int partIndex;
    int validatedPartIndex = 0;
    if (reg->partCount != 0)
        do
            validatedPartIndex++;
        while (validatedPartIndex != reg->partCount);
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

RegisterUse *RegisterUseList::findByRegister(RegisterEntry *registerEntry)
{
    return find(registerEntry);
}

RegisterUse *RegisterUseList::firstReal()
{
    RegisterUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        if (!registerEntry) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (!registerEntry->isVirtual)
            break;
    }
    return previousUse;
}

RegisterUse *RegisterUse::nextReal()
{
    RegisterUse *previousUse;
    for (previousUse = next; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        if (!registerEntry) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (!registerEntry->isVirtual)
            break;
    }
    return previousUse;
}

RegisterUse *RegisterUseList::firstVirtual()
{
    RegisterUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        if (!registerEntry) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (registerEntry->isVirtual)
            break;
    }
    return previousUse;
}

RegisterUse *RegisterUse::nextVirtual()
{
    RegisterUse *previousUse;
    for (previousUse = next; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        if (!registerEntry) {
            reportError("regref has no register!\n");
            exitAfterCleanup(-1);
        }
        if (registerEntry->isVirtual)
            break;
    }
    return previousUse;
}

bool RegisterUseList::optReset()
{
    for (RegisterUse *registerUse = head; registerUse; registerUse = registerUse->next) {
        for (int partIndex = 0; partIndex != 4; partIndex++) {
            registerUse->slots[partIndex].assignmentPayload = 0;
            registerUse->slots[partIndex].registerLock.resetLock();
        }
    }
    return true;
}

bool RegisterUse::optReset()
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

bool regUseSlot::addOutput(OperandLocation *operandLocation)
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

RegisterUse *RegisterUse::Clone()
{
    RegisterEntry *reg = registerEntry;
    if (!reg) {
        reportError("regref::Clone - input had no register\n");
        return 0;
    }
    RegisterUse *registerUse = new RegisterUse(reg);
    if (!registerUse) {
        reportError("failed to get new regref in clone\n");
        return 0;
    }
    registerUse->recordedOutputWriteCount = recordedOutputWriteCount;
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
    registerUse->copiedRegisterUseMetadata = copiedRegisterUseMetadata;
    registerUse->copiedRegisterUseFlag = copiedRegisterUseFlag;
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

const char *RegisterUse::regName()
{
    if (!registerEntry)
        return "REGREG_NOREG";
    return registerEntry->registerName();
}

OperandLocation *regUseSlot::findPreviousOutputAtPosition(int pos, bool all)
{
    if (all)
        pos = 0xff;
    OperandLocation *prev = 0;
    for (OperandLocation *currentLocation = outputLocations.head; currentLocation; currentLocation = currentLocation->next) {
        if (pos < currentLocation->cycleOffset)
            break;
        prev = currentLocation;
    }
    return prev;
}

int RegisterUseList::countUncoveredRegisterClassUses(int regClass)
{
    int uncoveredUseCount = 0;
    for (RegisterUse *previousUse = head; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        if (registerEntry->registerClass != regClass || registerEntry == registerEntry->ownerPool->zeroRegister)
            continue;
        if (~previousUse->inputMask & previousUse->throughputMask)
            uncoveredUseCount++;
        else if (!previousUse->outputMask)
            uncoveredUseCount++;
    }
    return uncoveredUseCount;
}

void RegisterUseList::dumpVirtualRegisterUses()
{
    if (hasConsistentCount()) {
        for (int poolIndex = 0; poolIndex != 12; poolIndex++) {
            RegPool *pool = getRegisterPoolByIndex(poolIndex);
            if (pool && pool->virtualPartCount) {
                reportInfo("%s\n", pool->poolName);
                dumpRegisterClassAllocations(poolIndex);
            }
        }
    }
}

static inline bool isVirtualRegister(RegisterEntry *registerEntry)
{
    if (registerEntry->isVirtual == true)
        return true;
    return false;
}

void RegisterUseList::dumpRegisterClassAllocations(int regClass)
{
    RegisterUse *previousUse;
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        RegisterEntry *phys = registerEntry->assignedPhysicalRegister;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->registerName();
        if (!isVirtualRegister(registerEntry))
            alloc = registerEntry->registerName();
        int mask = previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask | previousUse->throughputMask;
        if (registerEntry->registerClass == regClass && registerEntry != registerEntry->ownerPool->zeroRegister && previousUse->isExternal()) {
            const char *thr = "";
            if (previousUse->throughputMask & ~(previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask))
                thr = "-THR";
            reportInfo(" EXTERNAL: %20s - %s %s\n", registerEntry->maskName(mask), alloc, thr);
        }
    }
    for (previousUse = head; previousUse; previousUse = previousUse->next) {
        RegisterEntry *registerEntry = previousUse->registerEntry;
        RegisterEntry *phys = registerEntry->assignedPhysicalRegister;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->registerName();
        if (!isVirtualRegister(registerEntry))
            alloc = registerEntry->registerName();
        int mask = previousUse->inputMask | previousUse->outputMask | previousUse->externalInputMask | previousUse->externalOutputMask | previousUse->throughputMask;
        if (registerEntry->registerClass == regClass && registerEntry != registerEntry->ownerPool->zeroRegister && !previousUse->isExternal())
            reportInfo(" LOCAL : %20s - %s\n", registerEntry->maskName(mask), alloc);
    }
}
