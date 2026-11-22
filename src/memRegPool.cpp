#include "memRegPool.h"
#include "ol_s_inline.h"
#include "os.h"

memRegPool::memRegPool()
{
    unk_0e = 2;
}

memRegPool::~memRegPool()
{
}

bool func_08088f20(RegPool *pool, const char *base, int size)
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
    RegPoolEntry *tab = new RegPoolEntry[size];
    pool->registerEntries = tab;
    if (!tab) {
        extern const char memRegPoolAllocError[];
        reportError(memRegPoolAllocError);
        return false;
    }
    RegPoolEntry *e = tab;
    pool->entryCount = size;
    int idx = 10000;
    for (int i = 0; i != size; i++, e++, idx++) {
        formatIntoBuffer(buf, "(%s+%d)(vi00)", base, i);
        if (!e->addRegisterNameAlias(buf)) {
            reportError("init mem area: failed to add register %s\n", buf);
            return false;
        }
        e->allocatable = true;
        e->registerIndex = idx;
        e->registerClass = 11;
        e->ownerPool = pool;
        e->partCount = 4;
    }
    return true;
}

bool memRegPool::func_08088e54(workTable *)
{
    return true;
}

bool memRegPool::assignExt(instStream *)
{
    return true;
}

bool memRegPool::func_08088e6c(instStream *, RegPoolEntry *)
{
    return true;
}
