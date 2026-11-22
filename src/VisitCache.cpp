#include <stdlib.h>
#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"

static inline bool vc_release_inl(VisitCache *vc)
{
    if (vc->recordData)
        free(vc->recordData);
    vc->recordData = 0;
    vc->wordsPerRecord = 0;
    vc->bitsPerRecord = 0;
    vc->recordCapacity = 0;
    vc->recordCount = 0;
    return true;
}

static inline bool vc_extend_inl(VisitCache *vc, int rows)
{
    unsigned *p = (unsigned *)realloc(vc->recordData, rows * vc->wordsPerRecord * sizeof(unsigned));
    if (!p) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(vc);
        return false;
    }
    vc->recordData = p;
    vc->recordCapacity = rows;
    return true;
}

VisitCache::VisitCache()
{
    recordStarted = false;
    recordData = 0;
    wordsPerRecord = 0;
    bitsPerRecord = 0;
    recordCapacity = 0;
    recordCount = 0;
}

VisitCache::~VisitCache()
{
    vc_release_inl(this);
}

bool VisitCache::init(int nbits)
{
    vc_release_inl(this);
    bitsPerRecord = nbits;
    wordsPerRecord = (nbits + 31) / 32;
    if (!vc_extend_inl(this, 512)) {
        reportError("VisitCache:: init failed\n");
        vc_release_inl(this);
        return false;
    }
    return true;
}

bool VisitCache::release()
{
    if (recordData)
        free(recordData);
    recordData = 0;
    wordsPerRecord = 0;
    bitsPerRecord = 0;
    recordCapacity = 0;
    recordCount = 0;
    return true;
}

bool VisitCache::Extend(int rows)
{
    unsigned *p = (unsigned *)realloc(recordData, rows * wordsPerRecord * sizeof(unsigned));
    if (!p) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(this);
        return false;
    }
    recordData = p;
    recordCapacity = rows;
    return true;
}

bool VisitCache::startRecord()
{
    if (recordCount >= recordCapacity - 1) {
        if (!Extend(recordCapacity + 512)) {
            reportError("startRecord - failed to extend\n");
            return false;
        }
    }
    if (!recordData) {
        reportError("VisitCache:: startRecord - null table pointer\n");
        return false;
    }
    unsigned *row = recordData + recordCount * wordsPerRecord;
    recordStarted = true;
    for (int i = 0; i != wordsPerRecord; i++)
        *row++ = 0;
    return true;
}

bool VisitCache::setBit(int bit)
{
    unsigned *row = recordData + recordCount * wordsPerRecord;
    row[bit / 32] |= 1 << (bit & 31);
    return true;
}

int VisitCache::endRecord()
{
    int n = recordCount;
    unsigned *cur = recordData + n * wordsPerRecord;
    unsigned *row = recordData;
    int i;
    for (i = 0; i != n; i++) {
        int j;
        for (j = 0; j != wordsPerRecord; j++)
            if (row[j] != cur[j])
                break;
        if (j == wordsPerRecord)
            return i;
        row += wordsPerRecord;
    }
    recordCount = n + 1;
    return i;
}

bool VisitCache::testBit(int rec, int bit)
{
    unsigned *row = recordData + rec * wordsPerRecord;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void VisitCache::Dump()
{
    reportInfo("VisitCache::Dump()\n");
    unsigned *row = recordData;
    for (int i = 0; i != recordCount; i++) {
        reportMessage(" [%3d] ", i);
        for (int j = 0; j != wordsPerRecord; j++)
            reportMessage(" %08x", row[j]);
        reportMessage("\n");
        row += wordsPerRecord;
    }
    reportInfo("VisitCache:: %d records\n", recordCount);
}

static inline bool testPathBit(VisitCache *vc, int rec, int bit)
{
    unsigned *row = vc->recordData + rec * vc->wordsPerRecord;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void dumpPathKey(Program *p, int key)
{
    reportInfo("dumping path key '%d'\n", key);
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (testPathBit(&p->backTraceVisitCache, key, s->streamId))
            reportInfo("    %d: %s\n", s->streamName());
    }
    reportInfo(" done dumping path key '%d'\n", key);
}
