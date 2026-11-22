#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct OpcodeDescriptor;
OpcodeDescriptor* findOpcodeInfoByMnemonic(const char* name, bool);
bool asciiCaseEqual(const char* a, const char* b);

const char *regNameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]_$";

const char *regTypeName(int type)
{
    const char *typeName = "UNKNOWN_TYPE";
    switch (type) {
    case 8:  typeName = "VPU_I"; break;
    case 1:  typeName = "CPU_GPR"; break;
    case 2:  typeName = "FPR"; break;
    case 6:  typeName = "CPU_HW"; break;
    case 9:  typeName = "VPU_VF"; break;
    case 10: typeName = "VPU_HW"; break;
    }
    return typeName;
}

RegPool::RegPool()
{
    registerPrefix = 0;
    poolName = 0;
    registerEntries = 0;
    entryCount = 0;
    registerClass = 0;
    virtualPartCount = 0;
    virtualTail = 0;
    virtualHead = 0;
    virtualCount = 0;
    zeroRegister = 0;
    externalRegister = 0;
    poolKind = 0;
}

RegPool::~RegPool()
{
    RegisterEntry* registerEntry = virtualHead;
    while (registerEntry) {
        RegisterEntry* nextRegisterEntry = registerEntry->next;
        delete registerEntry;
        registerEntry = nextRegisterEntry;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    delete[] registerEntries;
}

bool RegPool::initializeRegisterPool(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts)
{
    this->registerPrefix = registerPrefix;
    virtualPartCount = virtualRegisterParts;
    poolName = name;
    registerEntries = 0;
    entryCount = 0;
    virtualTail = 0;
    virtualHead = 0;
    virtualCount = 0;
    registerClass = cls;

    int registerCount = 0;
    int descriptorIndex;
    RegisterEntry* entry;
    RegDesc* descriptor;
    for (descriptor = regDescTable; descriptor->names[0]; descriptor++)
        if (descriptor->registerClass == cls)
            registerCount++;
    if (!registerCount)
        return true;

    RegisterEntry* tab = new RegisterEntry[registerCount];
    registerEntries = tab;
    if (!tab) {
        reportError("RegPool::Init - couldn't allocate table\n");
        return false;
    }
    entry = tab;
    entryCount = registerCount;

    descriptor = regDescTable;
    descriptorIndex = 0;
    int classRunState = 0;
    for (; descriptor->names[0]; descriptor++, descriptorIndex++) {
        if (descriptor->registerClass != cls) {
            if (classRunState)
                classRunState = 2;
            continue;
        }
        if (classRunState == 2) {
            reportError("misordered uid\n");
            return false;
        }
        classRunState = 1;
        for (int nameIndex = 0; nameIndex != 4; nameIndex++) {
            if (descriptor->names[nameIndex] && !entry->addRegisterNameAlias(descriptor->names[nameIndex])) {
                reportError("failed to add register alias!\n");
                return false;
            }
        }
        entry->allocatable = descriptor->allocatable != 0;
        entry->registerClass = descriptor->registerClass;
        entry->ownerPool = this;
        entry->partCount = descriptor->partCount;
        entry->tracksHotOutputPaths = descriptor->tracksHotOutputPaths != 0;
        entry->nonBlockId = descriptor->nonBlockId;
        if (entry->nonBlockId && entry->partCount != 1) {
            reportError("nonblockid is set, but nparts!=1.. register is %s\n", entry->registerName());
            return false;
        }
        entry->registerIndex = descriptorIndex;
        entry++;
    }
    return true;
}

void RegPool::Dump()
{
    if (!poolName) {
        reportError("RegPool::Dump() POOL_WITH_NO_NAME\n");
        return;
    }
    int current = 0;
    if (registerEntries) {
        int registerIndex = 0;
        for (; current != entryCount; current = ++registerIndex) {
            RegisterEntry* registerEntry = &registerEntries[registerIndex];
            int offset = (char*)registerEntry - (char*)registerEntries;
            for (int unusedOffsetIteration = offset; unusedOffsetIteration; unusedOffsetIteration = 0)
                ;
        }
        if (virtualPartCount) {
            for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
                ;
        }
    }
}

RegisterEntry* RegPool::findByRegisterIndex(int index)
{
    if (!registerEntries)
        return 0;
    int registerOffset = index - registerEntries->registerIndex;
    if (registerOffset < 0 || registerOffset >= entryCount)
        return 0;
    return &registerEntries[registerOffset];
}

RegisterEntry* RegPool::findPhysicalByName(const char* name)
{
    RegisterEntry* registerEntry = registerEntries;
    for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
        if (registerEntry->nameAliases.findByNameIgnoreCase(name))
            return registerEntry;
    return 0;
}

bool RegPool::resetPoolThroughputState()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->resetRegisterThroughputState();
        for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
            virtualRegister->resetRegisterThroughputState();
    }
    return true;
}

bool RegPool::resetPoolPartTiming()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->resetRegisterPartTiming();
        for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
            virtualRegister->resetRegisterPartTiming();
    }
    return true;
}

bool RegPool::resetPoolFlowState()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->resetRegisterFlowState();
    }
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
        virtualRegister->resetRegisterFlowState();
    return true;
}

bool RegPool::clearPoolPartPayloads()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->clearRegisterPartPayloads();
        for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
            virtualRegister->clearRegisterPartPayloads();
    }
    return true;
}

bool RegPool::clearVirtualRegistersForParse()
{
    RegisterEntry* virtualRegister = virtualHead;
    while (virtualRegister) {
        RegisterEntry* nextVirtualRegister = virtualRegister->next;
        delete virtualRegister;
        virtualRegister = nextVirtualRegister;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    RegisterEntry* registerEntry = registerEntries;
    for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++) {
        registerEntry->enabledPhysicalRegister = false;
        registerEntry->isVirtual = false;
    }
    return true;
}

int isReservedRegisterName(const char* name)
{
    return RegPool::isReservedNameForClass(name, -123);
}

static inline bool isRegNameChar(char character)
{
    for (char* allowedCharacter = (char*)regNameChars; *allowedCharacter != '\0'; allowedCharacter++)
        if (*allowedCharacter == character)
            return true;
    return false;
}

bool RegPool::isReservedNameForClass(const char* name, int cls)
{
    if (!name) {
        reportError("isResvd. passed a null string as name\n");
        exitAfterCleanup(-1);
    }
    const char* nameCursor;
    for (nameCursor = name; *nameCursor; nameCursor++) {
        char nameCharacter = *nameCursor;
        if (!isRegNameChar(nameCharacter)) {
            reportUserMessage("can't use '%c' in the name of a register : '%s'\n", nameCharacter, name);
            return true;
        }
    }
    unsigned int len = nameCursor - name;
    if (len > 62) {
        reportUserMessage("register name '%s' is too long\n", name);
        return true;
    }
    if (len > 1 && findOpcodeInfoByMnemonic(name, true)) {
        reportUserMessage("invalid name for register '%s'\n", name);
        return true;
    }
    if (cls == 8) {
        if (asciiCaseEqual(name, "i") || asciiCaseEqual(name, "p")
            || asciiCaseEqual(name, "q") || asciiCaseEqual(name, "r"))
            return false;
    }
    for (RegDesc* descriptor = regDescTable; descriptor->names[0]; descriptor++) {
        if (descriptor->registerClass == cls)
            continue;
        for (int nameIndex = 0; nameIndex != 3; nameIndex++)
            if (descriptor->names[nameIndex] && asciiCaseEqual(name, descriptor->names[nameIndex]))
                return true;
    }
    return false;
}

bool RegPool::cycleRegisters(bool flag)
{
    RegisterEntry* registerEntry = registerEntries;
    for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
        if (!registerEntry->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", registerEntry->registerName());
            return false;
        }
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
        if (!virtualRegister->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", virtualRegister->registerName());
            return false;
        }
    return true;
}

bool RegPool::clearVirtualRegisters()
{
    RegisterEntry* registerEntry = virtualHead;
    while (registerEntry) {
        RegisterEntry* nextRegisterEntry = registerEntry->next;
        delete registerEntry;
        registerEntry = nextRegisterEntry;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    return true;
}

static inline RegisterEntry* findVirtual(RegisterEntry* registerEntry, const char* name)
{
    for (; registerEntry; registerEntry = registerEntry->next)
        if (asciiCaseEqual(registerEntry->registerName(), name))
            break;
    return registerEntry;
}

RegisterEntry* RegPool::findOrCreateRegister(const char* name)
{
    if (!virtualPartCount) {
        RegisterEntry* registerEntry = findPhysicalByName(name);
        if (!registerEntry)
            reportError("failed to find reg, and don't allow virtuals - pool %s regname=%s\n", poolName, name);
        return registerEntry;
    }
    RegisterEntry* registerEntry = findPhysicalByName(name);
    if (registerEntry)
        return registerEntry;
    if (isReservedNameForClass(name, registerClass)) {
        reportUserMessage("can't use %s as a name for one of the %s\n", name, poolName);
        return 0;
    }
    registerEntry = findVirtual(virtualHead, name);
    if (!registerEntry) {
        registerEntry = createVirtualRegister(name);
        if (!registerEntry) {
            reportError("failed to create a virtual reg\n");
            return 0;
        }
    }
    return registerEntry;
}

RegisterEntry* RegPool::findVirtualByName(const char* name)
{
    return findVirtual(virtualHead, name);
}

RegisterEntry* RegPool::createVirtualRegister(const char* name)
{
    if (isReservedNameForClass(name, registerClass))
        reportWarning(" name '%s' was reserved\n", name);
    RegisterEntry* registerEntry;
    for (registerEntry = virtualHead; registerEntry; registerEntry = registerEntry->next)
        if (asciiCaseEqual(registerEntry->registerName(), name))
            break;
    if (registerEntry) {
        reportWarning("WARNING: AddVirtual : name already found '%s'!\n", name);
        return 0;
    }
    registerEntry = new RegisterEntry;
    if (!registerEntry) {
        reportError("failed to get a new virtual register\n");
        return 0;
    }
    registerEntry->ownerPool = this;
    registerEntry->registerClass = registerClass;
    registerEntry->previous = 0;
    registerEntry->next = 0;
    registerEntry->partCount = virtualPartCount;
    registerEntry->registerIndex = registerClass * 2000 + virtualCount;
    registerEntry->isVirtual = true;
    registerEntry->nonBlockId = 0;
    if (!registerEntry->addRegisterNameAlias(name)) {
        reportError("failed to christen name to register %s \n", name);
        delete registerEntry;
        return 0;
    }
    if (virtualHead) {
        virtualTail->next = registerEntry;
        registerEntry->previous = virtualTail;
        virtualTail = registerEntry;
    } else {
        virtualTail = registerEntry;
        virtualHead = registerEntry;
    }
    virtualCount++;
    return registerEntry;
}

bool RegPool::markAllocatablePhysicalRegistersUsed()
{
    if (!registerEntries) {
        reportError("regPool: add all to use - but had no table\n");
        return false;
    }
    RegisterEntry* registerEntry = registerEntries;
    for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
        if (registerEntry->allocatable)
            registerEntry->enabledPhysicalRegister = true;
    return true;
}

bool RegPool::clearRegisterAssignmentsFirstPass()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->assignedPhysicalRegister = 0;
    }
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
        virtualRegister->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::clearRegisterAssignmentsSecondPass()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->assignedPhysicalRegister = 0;
    }
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
        virtualRegister->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::resetPoolLocalAllocationState()
{
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next) {
        if (!virtualRegister->clearPartPayloadMarkers())
            goto fail;
        virtualRegister->assignedPhysicalRegister = 0;
        virtualRegister->assignmentCostTableSize = -1;
    }
    {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++) {
            if (!registerEntry->clearPartPayloadMarkers())
                goto fail;
            registerEntry->assignedPhysicalRegister = 0;
        }
    }
    return true;
fail:
    reportError("failed to resetlalloc\n");
    return false;
}

int maximumAllocationParts;
int allocationRegisterCount;
int allocationTimelineStart;
int allocationTimelineEnd;
char allocationRegisterTimeline[0x20000];
char candidateRegisterTimeline[0x1000];
char allocationRegisterMask[32];
int moveRegCount;

RegisterEntry* RegPool::findMoveRegister(Instr* move)
{
    char buf[128];
    formatIntoBuffer(buf, "move__m%d", moveRegCount);
    moveRegCount++;
    if (!move) {
        reportError("getMoveReg -  but no move\n");
        exitAfterCleanup(-1);
    }
    RegisterEntry* registerEntry = createVirtualRegister(buf);
    if (!registerEntry) {
        reportError("failed to add a new virtual register!\n");
        exitAfterCleanup(-1);
    }
    if (registerEntry->moveInstruction) {
        reportError("move inst already set in getMoveReg\n");
        exitAfterCleanup(-1);
    }
    registerEntry->moveInstruction = move;
    return registerEntry;
}

bool RegPool::ownsRegisterClass(RegisterEntry* registerEntry)
{
    return registerEntry && registerEntry->registerClass == registerClass;
}

bool RegPool::optReset()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->optReset();
        for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
            virtualRegister->optReset();
    }
    return true;
}

bool RegPool::emitReset()
{
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            registerEntry->usedSlotMask = 0;
        for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next)
            virtualRegister->usedSlotMask = 0;
    }
    return true;
}

char* RegPool::emitStats(char* outputBuffer)
{
    outputBuffer += formatIntoBuffer(outputBuffer, "; %3d %s\n", countUsedRegisters(), poolName);
    return outputBuffer;
}

int RegPool::regIndex(int reg)
{
    int registerOffset = -1;
    if (registerEntries) {
        reg -= registerEntries->registerIndex;
        if (reg >= 0 && reg < entryCount)
            registerOffset = reg;
    }
    return registerOffset;
}

int RegPool::countUsedRegisters()
{
    int usedRegisterCount = 0;
    if (registerEntries) {
        RegisterEntry* registerEntry = registerEntries;
        for (int entryIndex = 0; entryIndex != entryCount; entryIndex++, registerEntry++)
            if (registerEntry->usedSlotMask)
                usedRegisterCount++;
    }
    return usedRegisterCount;
}

int RegPool::countMarkedPhysicalRegisters()
{
    int enabledRegisterCount = 0;
    if (registerEntries) {
        for (int registerIndex = 0; registerIndex != entryCount; registerIndex++)
            if (registerEntries[registerIndex].enabledPhysicalRegister)
                enabledRegisterCount++;
    }
    return enabledRegisterCount;
}

int RegisterEntry::gatherInflight(OperandLocation** out)
{
    int inflightLocationCount = 0;
    int partIndex;
    for (partIndex = 0; partIndex != partCount; partIndex++) {
        RegisterPartState* registerPart = getRegisterPartByIndex(partIndex);
        OperandLocation* location = &registerPart->currentLocation;
        if (location->cycleOffset)
            out[inflightLocationCount++] = location;
        int pendingLocationIndex;
        OperandLocation* pendingLocation;
        for (pendingLocationIndex = 0, pendingLocation = registerPart->pendingLocations; pendingLocationIndex != 4; pendingLocationIndex++, pendingLocation++)
            if (pendingLocation->cycleOffset)
                out[inflightLocationCount++] = pendingLocation;
    }
    return inflightLocationCount;
}

int RegPool::gatherInflight(OperandLocation** out)
{
    int inflightLocationCount = 0;
    for (int registerIndex = 0; registerIndex != entryCount; registerIndex++) {
        RegisterEntry* registerEntry = &registerEntries[registerIndex];
        int appendedLocationCount = registerEntry->gatherInflight(out + inflightLocationCount);
        if (appendedLocationCount < 0)
            return -1;
        inflightLocationCount += appendedLocationCount;
    }
    for (RegisterEntry* virtualRegister = virtualHead; virtualRegister; virtualRegister = virtualRegister->next) {
        int virtualInflightCount = virtualRegister->gatherInflight(out + inflightLocationCount);
        if (virtualInflightCount < 0)
            return -1;
        inflightLocationCount += virtualInflightCount;
    }
    return inflightLocationCount;
}
