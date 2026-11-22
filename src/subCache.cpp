#include "os.h"
#include "subCache.h"

static inline void subCache_release_inl(subCache *sc)
{
    if (sc->cachedValues)
        delete[] sc->cachedValues;
    sc->cachedValues = 0;
    sc->maximumFlowId = 0;
    sc->wordsPerFlowRow = 0;
}

static inline void subCache_flush_inl(subCache *sc)
{
    if (!sc->cachedValues)
        exitWithError("subcache flush - no table\n");
    for (unsigned i = 0; i != sc->allocatedWords; i++)
        sc->cachedValues[i] = 0;
}

void subCache::init(int fuidMax)
{
    subCache_release_inl(this);
    if (fuidMax <= 0)
        exitWithError("fuid_max is <=0 in sub cache init\n");
    wordsPerFlowRow = (unsigned)(fuidMax + 3) >> 2;
    allocatedWords = wordsPerFlowRow * (fuidMax + 1);
    maximumFlowId = fuidMax;
    cachedValues = new int[allocatedWords];
    if (!cachedValues)
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
    if (!cachedValues)
        exitWithError("no table set for subCache\n");
    if (fuid > maximumFlowId)
        exitWithError("out of bounds fuid for subCache\n");
    return fuid * wordsPerFlowRow * 4;
}
