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
    regUse *ru = head;
    while (ru) {
        regUse *next = ru->next;
        if (ru->next)
            ru->next->previous = ru->previous;
        else
            tail = ru->previous;
        if (ru->previous)
            ru->previous->next = ru->next;
        else
            head = ru->next;
        count--;
        ru->previous = 0;
        ru->next = 0;
        delete ru;
        ru = next;
    }
}

bool regUseList::clear()
{
    regUse *ru = head;
    while (ru) {
        regUse *next = ru->next;
        if (ru->next)
            ru->next->previous = ru->previous;
        else
            tail = ru->previous;
        if (ru->previous)
            ru->previous->next = ru->next;
        else
            head = ru->next;
        count--;
        ru->previous = 0;
        ru->next = 0;
        delete ru;
        ru = next;
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
    regUse *ru = new regUse(reg);
    if (!ru) {
        reportError(regUseNewFailedError);
        exitAfterCleanup(-1);
        return 0;
    }
    ru->previous = 0;
    ru->next = 0;
    if (!tail) {
        head = ru;
    } else {
        tail->next = ru;
        ru->previous = tail;
    }
    tail = ru;
    count++;
    return ru;
}

bool regUseList::append(regUse *ru)
{
    ru->previous = 0;
    ru->next = 0;
    if (!tail) {
        head = ru;
    } else {
        tail->next = ru;
        ru->previous = tail;
    }
    tail = ru;
    count++;
    return true;
}

bool regUseList::remove(regUse *ru)
{
    if (ru->next)
        ru->next->previous = ru->previous;
    else
        tail = ru->previous;
    if (ru->previous)
        ru->previous->next = ru->next;
    else
        head = ru->next;
    count--;
    ru->previous = 0;
    ru->next = 0;
    return true;
}

bool regUseList::hasConsistentCount()
{
    int n = 0;
    for (regUse *ru = head; ru; ru = ru->next) {
        if (!ru->registerEntry)
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
