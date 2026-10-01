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
        func_0804ad38("initmemarea: well - no base_ea set!\n");
        return false;
    }
    if (size <= 0) {
        func_0804ad38("init mem area: size was <=0\n");
        return false;
    }
    RegPoolEntry *tab = new RegPoolEntry[size];
    pool->unk_04 = tab;
    if (!tab) {
        func_0804ad38("RegPool::Init - couldn't allocate table\n");
        return false;
    }
    RegPoolEntry *e = tab;
    pool->unk_08 = size;
    int idx = 10000;
    for (int i = 0; i != size; i++, e++, idx++) {
        func_0804adbc(buf, "(%s+%d)(vi00)", base, i);
        if (!e->func_0805a8a4(buf)) {
            func_0804ad38("init mem area: failed to add register %s\n", buf);
            return false;
        }
        e->unk_15 = true;
        e->unk_30 = idx;
        e->unk_24 = 11;
        e->unk_04 = pool;
        e->unk_28 = 4;
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
