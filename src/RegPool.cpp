#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct iInfo;
iInfo* findOpcodeInfoByMnemonic(const char* name, bool);
bool asciiCaseEqual(const char* a, const char* b);

const char *regNameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]_$";

const char *regTypeName(int type)
{
    const char *s = "UNKNOWN_TYPE";
    switch (type) {
    case 8:  s = "VPU_I"; break;
    case 1:  s = "CPU_GPR"; break;
    case 2:  s = "FPR"; break;
    case 6:  s = "CPU_HW"; break;
    case 9:  s = "VPU_VF"; break;
    case 10: s = "VPU_HW"; break;
    }
    return s;
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
    RegPoolEntry* registerEntry = virtualHead;
    while (registerEntry) {
        RegPoolEntry* n = registerEntry->next;
        delete registerEntry;
        registerEntry = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    delete[] registerEntries;
}

bool RegPool::Init(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts)
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
    RegPoolEntry* entry;
    RegDesc* descriptor;
    for (descriptor = regDescTable; descriptor->names[0]; descriptor++)
        if (descriptor->registerClass == cls)
            registerCount++;
    if (!registerCount)
        return true;

    RegPoolEntry* tab = new RegPoolEntry[registerCount];
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
        for (int i = 0; i != 4; i++) {
            if (descriptor->names[i] && !entry->addRegisterNameAlias(descriptor->names[i])) {
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
        int i = 0;
        for (; current != entryCount; current = ++i) {
            RegPoolEntry* registerEntry = &registerEntries[i];
            int offset = (char*)registerEntry - (char*)registerEntries;
            for (int p = offset; p; p = 0)
                ;
        }
        if (virtualPartCount) {
            for (RegPoolEntry* p = virtualHead; p; p = p->next)
                ;
        }
    }
}

RegPoolEntry* RegPool::findByRegisterIndex(int index)
{
    if (!registerEntries)
        return 0;
    int i = index - registerEntries->registerIndex;
    if (i < 0 || i >= entryCount)
        return 0;
    return &registerEntries[i];
}

RegPoolEntry* RegPool::findPhysicalByName(const char* name)
{
    RegPoolEntry* registerEntry = registerEntries;
    for (int i = 0; i != entryCount; i++, registerEntry++)
        if (registerEntry->nameAliases.findByNameIgnoreCase(name))
            return registerEntry;
    return 0;
}

bool RegPool::resetPoolThroughputState()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->resetRegisterThroughputState();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->resetRegisterThroughputState();
    }
    return true;
}

bool RegPool::resetPoolPartTiming()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->resetRegisterPartTiming();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->resetRegisterPartTiming();
    }
    return true;
}

bool RegPool::resetPoolFlowState()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->resetRegisterFlowState();
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->resetRegisterFlowState();
    return true;
}

bool RegPool::clearPoolPartPayloads()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->clearRegisterPartPayloads();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->clearRegisterPartPayloads();
    }
    return true;
}

bool RegPool::clearVirtualRegistersForParse()
{
    RegPoolEntry* p = virtualHead;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    RegPoolEntry* registerEntry = registerEntries;
    for (int i = 0; i != entryCount; i++, registerEntry++) {
        registerEntry->enabledPhysicalRegister = false;
        registerEntry->isVirtual = false;
    }
    return true;
}

int isReservedRegisterName(const char* name)
{
    return RegPool::isResvd(name, -123);
}

static inline bool isRegNameChar(char c)
{
    for (char* q = (char*)regNameChars; *q != '\0'; q++)
        if (*q == c)
            return true;
    return false;
}

bool RegPool::isResvd(const char* name, int cls)
{
    if (!name) {
        reportError("isResvd. passed a null string as name\n");
        exitAfterCleanup(-1);
    }
    const char* p;
    for (p = name; *p; p++) {
        char c = *p;
        if (!isRegNameChar(c)) {
            reportUserMessage("can't use '%c' in the name of a register : '%s'\n", c, name);
            return true;
        }
    }
    unsigned int len = p - name;
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
        for (int i = 0; i != 3; i++)
            if (descriptor->names[i] && asciiCaseEqual(name, descriptor->names[i]))
                return true;
    }
    return false;
}

bool RegPool::cycleRegisters(bool flag)
{
    RegPoolEntry* registerEntry = registerEntries;
    for (int i = 0; i != entryCount; i++, registerEntry++)
        if (!registerEntry->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", registerEntry->registerName());
            return false;
        }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        if (!p->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", p->registerName());
            return false;
        }
    return true;
}

bool RegPool::clearVirtualRegisters()
{
    RegPoolEntry* registerEntry = virtualHead;
    while (registerEntry) {
        RegPoolEntry* n = registerEntry->next;
        delete registerEntry;
        registerEntry = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    return true;
}

static inline RegPoolEntry* findVirtual(RegPoolEntry* registerEntry, const char* name)
{
    for (; registerEntry; registerEntry = registerEntry->next)
        if (asciiCaseEqual(registerEntry->registerName(), name))
            break;
    return registerEntry;
}

RegPoolEntry* RegPool::findOrCreateRegister(const char* name)
{
    if (!virtualPartCount) {
        RegPoolEntry* registerEntry = findPhysicalByName(name);
        if (!registerEntry)
            reportError("failed to find reg, and don't allow virtuals - pool %s regname=%s\n", poolName, name);
        return registerEntry;
    }
    RegPoolEntry* registerEntry = findPhysicalByName(name);
    if (registerEntry)
        return registerEntry;
    if (isResvd(name, registerClass)) {
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

RegPoolEntry* RegPool::findVirtualByName(const char* name)
{
    return findVirtual(virtualHead, name);
}

RegPoolEntry* RegPool::createVirtualRegister(const char* name)
{
    if (isResvd(name, registerClass))
        reportWarning(" name '%s' was reserved\n", name);
    RegPoolEntry* registerEntry;
    for (registerEntry = virtualHead; registerEntry; registerEntry = registerEntry->next)
        if (asciiCaseEqual(registerEntry->registerName(), name))
            break;
    if (registerEntry) {
        reportWarning("WARNING: AddVirtual : name already found '%s'!\n", name);
        return 0;
    }
    registerEntry = new RegPoolEntry;
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
    RegPoolEntry* registerEntry = registerEntries;
    for (int i = 0; i != entryCount; i++, registerEntry++)
        if (registerEntry->allocatable)
            registerEntry->enabledPhysicalRegister = true;
    return true;
}

bool RegPool::clearRegisterAssignmentsFirstPass()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->assignedPhysicalRegister = 0;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::clearRegisterAssignmentsSecondPass()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->assignedPhysicalRegister = 0;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::resetPoolLocalAllocationState()
{
    for (RegPoolEntry* p = virtualHead; p; p = p->next) {
        if (!p->clearPartPayloadMarkers())
            goto fail;
        p->assignedPhysicalRegister = 0;
        p->assignmentCostTableSize = -1;
    }
    {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++) {
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

RegPoolEntry* RegPool::getMoveReg(Instr* move)
{
    char buf[128];
    formatIntoBuffer(buf, "move__m%d", moveRegCount);
    moveRegCount++;
    if (!move) {
        reportError("getMoveReg -  but no move\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry* registerEntry = createVirtualRegister(buf);
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

bool RegPool::ownsRegisterClass(RegPoolEntry* registerEntry)
{
    return registerEntry && registerEntry->registerClass == registerClass;
}

bool RegPool::optReset()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->optReset();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->optReset();
    }
    return true;
}

bool RegPool::emitReset()
{
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            registerEntry->usedSlotMask = 0;
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->usedSlotMask = 0;
    }
    return true;
}

char* RegPool::emitStats(char* buf)
{
    buf += formatIntoBuffer(buf, "; %3d %s\n", countUsed(), poolName);
    return buf;
}

int RegPool::regIndex(int reg)
{
    int r = -1;
    if (registerEntries) {
        reg -= registerEntries->registerIndex;
        if (reg >= 0 && reg < entryCount)
            r = reg;
    }
    return r;
}

int RegPool::countUsed()
{
    int n = 0;
    if (registerEntries) {
        RegPoolEntry* registerEntry = registerEntries;
        for (int i = 0; i != entryCount; i++, registerEntry++)
            if (registerEntry->usedSlotMask)
                n++;
    }
    return n;
}

int RegPool::countMarkedPhysicalRegisters()
{
    int n = 0;
    if (registerEntries) {
        for (int i = 0; i != entryCount; i++)
            if (registerEntries[i].enabledPhysicalRegister)
                n++;
    }
    return n;
}

int RegPoolEntry::gatherInflight(ol_s** out)
{
    int n = 0;
    int i;
    for (i = 0; i != partCount; i++) {
        rPart* registerPart = getRP(i);
        ol_s* location = &registerPart->currentLocation;
        if (location->cycleOffset)
            out[n++] = location;
        int j;
        ol_s* p;
        for (j = 0, p = registerPart->pendingLocations; j != 4; j++, p++)
            if (p->cycleOffset)
                out[n++] = p;
    }
    return n;
}

int RegPool::gatherInflight(ol_s** out)
{
    int n = 0;
    for (int i = 0; i != entryCount; i++) {
        RegPoolEntry* registerEntry = &registerEntries[i];
        int r = registerEntry->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next) {
        int r = p->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    return n;
}
