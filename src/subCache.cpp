#include "os.h"
#include "subCache.h"

static inline void subCache_release_inl(subCache *sc)
{
    if (sc->unk_00)
        delete[] sc->unk_00;
    sc->unk_00 = 0;
    sc->unk_04 = 0;
    sc->unk_08 = 0;
}

static inline void subCache_flush_inl(subCache *sc)
{
    if (!sc->unk_00)
        exitWithError("subcache flush - no table\n");
    for (unsigned i = 0; i != sc->unk_0c; i++)
        sc->unk_00[i] = 0;
}

void subCache::init(int fuidMax)
{
    subCache_release_inl(this);
    if (fuidMax <= 0)
        exitWithError("fuid_max is <=0 in sub cache init\n");
    unk_08 = (unsigned)(fuidMax + 3) >> 2;
    unk_0c = unk_08 * (fuidMax + 1);
    unk_04 = fuidMax;
    unk_00 = new int[unk_0c];
    if (!unk_00)
        exitWithError("failed to get table for subc\n");
    subCache_flush_inl(this);
}

void subCache::release()
{
    subCache_release_inl(this);
}

void subCache::flush()
{
    subCache_flush_inl(this);
}

int subCache::offset(unsigned fuid)
{
    if (!unk_00)
        exitWithError("no table set for subCache\n");
    if (fuid > unk_04)
        exitWithError("out of bounds fuid for subCache\n");
    return fuid * unk_08 * 4;
}
