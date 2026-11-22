#include "memRegPool.h"
#include "ol_s_inline.h"
#include "os.h"

memRegPool::memRegPool()
{
    poolKind = 2;
}

memRegPool::~memRegPool()
{
}

bool initializeMemoryRegisterArea(RegPool *pool, const char *base, int size)
{
    char buf[128];

    if (!base) {
        reportError("initmemarea: well - no base_ea set!\n");
        return false;
    }
    if (size <= 0) {
        reportError("init mem area: size was <=0\n");
        return false;
    }
    RegisterEntry *tab = new RegisterEntry[size];
    pool->registerEntries = tab;
    if (!tab) {
        extern const char memRegPoolAllocError[];
        reportError(memRegPoolAllocError);
        return false;
    }
    RegisterEntry *registerEntry = tab;
    pool->entryCount = size;
    int idx = 10000;
    for (int registerOffset = 0; registerOffset != size; registerOffset++, registerEntry++, idx++) {
        formatIntoBuffer(buf, "(%s+%d)(vi00)", base, registerOffset);
        if (!registerEntry->addRegisterNameAlias(buf)) {
            reportError("init mem area: failed to add register %s\n", buf);
            return false;
        }
        registerEntry->allocatable = true;
        registerEntry->registerIndex = idx;
        registerEntry->registerClass = 11;
        registerEntry->ownerPool = pool;
        registerEntry->partCount = 4;
    }
    return true;
}

bool memRegPool::assignRegistersForWorkTable(ScheduleTable * unusedTable)
{
    return true;
}

bool memRegPool::assignExt(InstructionStream * unusedStream)
{
    return true;
}

bool memRegPool::discardDisjointExternalRegister(InstructionStream *, RegisterEntry * unusedRegister)
{
    return true;
}
