#include <stdlib.h>
#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"

static inline bool vc_release_inl(VisitCache *vc)
{
    if (vc->unk_00)
        free(vc->unk_00);
    vc->unk_00 = 0;
    vc->unk_04 = 0;
    vc->unk_08 = 0;
    vc->unk_0c = 0;
    vc->unk_10 = 0;
    return true;
}

static inline bool vc_extend_inl(VisitCache *vc, int rows)
{
    unsigned *p = (unsigned *)realloc(vc->unk_00, rows * vc->unk_04 * sizeof(unsigned));
    if (!p) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(vc);
        return false;
    }
    vc->unk_00 = p;
    vc->unk_0c = rows;
    return true;
}

VisitCache::VisitCache()
{
    unk_14 = false;
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
}

VisitCache::~VisitCache()
{
    vc_release_inl(this);
}

bool VisitCache::init(int nbits)
{
    vc_release_inl(this);
    unk_08 = nbits;
    unk_04 = (nbits + 31) / 32;
    if (!vc_extend_inl(this, 512)) {
        reportError("VisitCache:: init failed\n");
        vc_release_inl(this);
        return false;
    }
    return true;
}

bool VisitCache::release()
{
    if (unk_00)
        free(unk_00);
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    return true;
}

bool VisitCache::Extend(int rows)
{
    unsigned *p = (unsigned *)realloc(unk_00, rows * unk_04 * sizeof(unsigned));
    if (!p) {
        reportError("VisitCache::Extend.. failed to realloc\n");
        vc_release_inl(this);
        return false;
    }
    unk_00 = p;
    unk_0c = rows;
    return true;
}

bool VisitCache::startRecord()
{
    if (unk_10 >= unk_0c - 1) {
        if (!Extend(unk_0c + 512)) {
            reportError("startRecord - failed to extend\n");
            return false;
        }
    }
    if (!unk_00) {
        reportError("VisitCache:: startRecord - null table pointer\n");
        return false;
    }
    unsigned *row = unk_00 + unk_10 * unk_04;
    unk_14 = true;
    for (int i = 0; i != unk_04; i++)
        *row++ = 0;
    return true;
}

bool VisitCache::setBit(int bit)
{
    unsigned *row = unk_00 + unk_10 * unk_04;
    row[bit / 32] |= 1 << (bit & 31);
    return true;
}

int VisitCache::endRecord()
{
    int n = unk_10;
    unsigned *cur = unk_00 + n * unk_04;
    unsigned *row = unk_00;
    int i;
    for (i = 0; i != n; i++) {
        int j;
        for (j = 0; j != unk_04; j++)
            if (row[j] != cur[j])
                break;
        if (j == unk_04)
            return i;
        row += unk_04;
    }
    unk_10 = n + 1;
    return i;
}

bool VisitCache::testBit(int rec, int bit)
{
    unsigned *row = unk_00 + rec * unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void VisitCache::Dump()
{
    reportInfo("VisitCache::Dump()\n");
    unsigned *row = unk_00;
    for (int i = 0; i != unk_10; i++) {
        reportMessage(" [%3d] ", i);
        for (int j = 0; j != unk_04; j++)
            reportMessage(" %08x", row[j]);
        reportMessage("\n");
        row += unk_04;
    }
    reportInfo("VisitCache:: %d records\n", unk_10);
}

static inline bool testPathBit(VisitCache *vc, int rec, int bit)
{
    unsigned *row = vc->unk_00 + rec * vc->unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void dumpPathKey(Program *p, int key)
{
    reportInfo("dumping path key '%d'\n", key);
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (testPathBit(&p->backTraceVisitCache, key, s->unk_2c0))
            reportInfo("    %d: %s\n", s->streamName());
    }
    reportInfo(" done dumping path key '%d'\n", key);
}
