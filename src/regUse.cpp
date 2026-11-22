#include "os.h"
extern const char regUseNewFailedError[];
#include "RegPool_shared.h"

regUse::regUse(RegPoolEntry *reg)
{
    this->registerEntry = reg;
    externalInputMask = 0;
    externalOutputMask = 0;
    inputMask = 0;
    outputMask = 0;
    throughputMask = 0;
    unk_11 = 0;
    hazardMask = 0;
    sourcePathHazardMask = 0;
    hazardBroadcastMask = 0;
    externalBlockMode = -1;
    previous = 0;
    next = 0;
    unk_18 = reg->unk_36;
    definingInstruction = reg->throughputWriter;
}

regUse::~regUse()
{
}

regUseList::regUseList()
{
    tail = 0;
    head = 0;
    count = 0;
}

regUseList::~regUseList()
{
    regUse *registerUse = head;
    while (registerUse) {
        regUse *next = registerUse->next;
        if (registerUse->next)
            registerUse->next->previous = registerUse->previous;
        else
            tail = registerUse->previous;
        if (registerUse->previous)
            registerUse->previous->next = registerUse->next;
        else
            head = registerUse->next;
        count--;
        registerUse->previous = 0;
        registerUse->next = 0;
        delete registerUse;
        registerUse = next;
    }
}

bool regUseList::clear()
{
    regUse *registerUse = head;
    while (registerUse) {
        regUse *next = registerUse->next;
        if (registerUse->next)
            registerUse->next->previous = registerUse->previous;
        else
            tail = registerUse->previous;
        if (registerUse->previous)
            registerUse->previous->next = registerUse->next;
        else
            head = registerUse->next;
        count--;
        registerUse->previous = 0;
        registerUse->next = 0;
        delete registerUse;
        registerUse = next;
    }
    return true;
}

regUse *regUseList::AddNewRegRef(RegPoolEntry *reg)
{
    if (find(reg)) {
        reportError("addnewregref - already have rg\n");
        exitAfterCleanup(-1);
        return 0;
    }
    regUse *registerUse = new regUse(reg);
    if (!registerUse) {
        reportError(regUseNewFailedError);
        exitAfterCleanup(-1);
        return 0;
    }
    registerUse->previous = 0;
    registerUse->next = 0;
    if (!tail) {
        head = registerUse;
    } else {
        tail->next = registerUse;
        registerUse->previous = tail;
    }
    tail = registerUse;
    count++;
    return registerUse;
}

bool regUseList::append(regUse *registerUse)
{
    registerUse->previous = 0;
    registerUse->next = 0;
    if (!tail) {
        head = registerUse;
    } else {
        tail->next = registerUse;
        registerUse->previous = tail;
    }
    tail = registerUse;
    count++;
    return true;
}

bool regUseList::remove(regUse *registerUse)
{
    if (registerUse->next)
        registerUse->next->previous = registerUse->previous;
    else
        tail = registerUse->previous;
    if (registerUse->previous)
        registerUse->previous->next = registerUse->next;
    else
        head = registerUse->next;
    count--;
    registerUse->previous = 0;
    registerUse->next = 0;
    return true;
}

bool regUseList::hasConsistentCount()
{
    int n = 0;
    for (regUse *registerUse = head; registerUse; registerUse = registerUse->next) {
        if (!registerUse->registerEntry)
            return false;
        n++;
    }
    return n == count;
}

bool regUseList::hasEntries()
{
    return tail != 0;
}

regUseSlot::regUseSlot()
{
    throughputSums[1] = 0;
    throughputSums[0] = 0;
    throughputSums[3] = 0;
    throughputSums[2] = 0;
    timingValues[0] = -1;
    timingValues[2] = -1;
    timingValues[1] = -1;
    timingValues[3] = -1;
    timingValues[4] = -1;
    timingValues[5] = -1;
    timingValues[6] = -1;
    timingValues[7] = -1;
    timingValues[8] = -1;
    timingValues[9] = -1;
    timingValues[10] = -1;
    timingValues[11] = -1;
    timingValues[12] = -1;
    earliestExternalCycle = -1;
    latestExternalCycle = -1;
    assignmentPayload = 0;
    registerLock.resetLock();
}
