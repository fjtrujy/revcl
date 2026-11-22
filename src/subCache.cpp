#include "os.h"
#include "subCache.h"

static inline void subCache_release_inl(subCache *cache)
{
    if (cache->cachedValues)
        delete[] cache->cachedValues;
    cache->cachedValues = 0;
    cache->maximumFlowId = 0;
    cache->wordsPerFlowRow = 0;
}

static inline void subCache_flush_inl(subCache *cache)
{
    if (!cache->cachedValues)
        exitWithError("subcache flush - no table\n");
    for (unsigned i = 0; i != cache->allocatedWords; i++)
        cache->cachedValues[i] = 0;
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
