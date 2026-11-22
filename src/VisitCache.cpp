#include <stdlib.h>
#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"

static inline bool vc_release_inl(VisitCache *visitCache)
{
    if (visitCache->recordData)
        free(visitCache->recordData);
    visitCache->recordData = 0;
    visitCache->wordsPerRecord = 0;
    visitCache->bitsPerRecord = 0;
    visitCache->recordCapacity = 0;
    visitCache->recordCount = 0;
    return true;
}

static inline bool vc_extend_inl(VisitCache *visitCache, int rows)
{
    unsigned *resizedRecordData = (unsigned *)realloc(visitCache->recordData, rows * visitCache->wordsPerRecord * sizeof(unsigned));
    if (!resizedRecordData) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(visitCache);
        return false;
    }
    visitCache->recordData = resizedRecordData;
    visitCache->recordCapacity = rows;
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

bool VisitCache::resizeRecordStorage(int rows)
{
    unsigned *resizedRecordData = (unsigned *)realloc(recordData, rows * wordsPerRecord * sizeof(unsigned));
    if (!resizedRecordData) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(this);
        return false;
    }
    recordData = resizedRecordData;
    recordCapacity = rows;
    return true;
}

bool VisitCache::startRecord()
{
    if (recordCount >= recordCapacity - 1) {
        if (!resizeRecordStorage(recordCapacity + 512)) {
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
    for (int wordIndex = 0; wordIndex != wordsPerRecord; wordIndex++)
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
    int recordIndex = recordCount;
    unsigned *cur = recordData + recordIndex * wordsPerRecord;
    unsigned *row = recordData;
    int comparisonIndex;
    for (comparisonIndex = 0; comparisonIndex != recordIndex; comparisonIndex++) {
        int wordIndex;
        for (wordIndex = 0; wordIndex != wordsPerRecord; wordIndex++)
            if (row[wordIndex] != cur[wordIndex])
                break;
        if (wordIndex == wordsPerRecord)
            return comparisonIndex;
        row += wordsPerRecord;
    }
    recordCount = recordIndex + 1;
    return comparisonIndex;
}

bool VisitCache::testBit(int rec, int bit)
{
    unsigned *row = recordData + rec * wordsPerRecord;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void VisitCache::dumpVisitRecords()
{
    reportInfo("VisitCache::Dump()\n");
    unsigned *row = recordData;
    for (int recordIndex = 0; recordIndex != recordCount; recordIndex++) {
        reportMessage(" [%3d] ", recordIndex);
        for (int wordIndex = 0; wordIndex != wordsPerRecord; wordIndex++)
            reportMessage(" %08x", row[wordIndex]);
        reportMessage("\n");
        row += wordsPerRecord;
    }
    reportInfo("VisitCache:: %d records\n", recordCount);
}

static inline bool testPathBit(VisitCache *visitCache, int rec, int bit)
{
    unsigned *row = visitCache->recordData + rec * visitCache->wordsPerRecord;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void dumpBacktracePathKey(Program *program, int key)
{
    reportInfo("dumping path key '%d'\n", key);
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (testPathBit(&program->backTraceVisitCache, key, stream->streamId))
            reportInfo("    %d: %s\n", stream->streamName());
    }
    reportInfo(" done dumping path key '%d'\n", key);
}
