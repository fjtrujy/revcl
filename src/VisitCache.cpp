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
        func_0804ad38("VisitCache::Extend.. failed to realloc\n");
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
        func_0804ad38("VisitCache:: init failed\n");
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
        func_0804ad38("VisitCache::Extend.. failed to realloc\n");
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
            func_0804ad38("startRecord - failed to extend\n");
            return false;
        }
    }
    if (!unk_00) {
        func_0804ad38("VisitCache:: startRecord - null table pointer\n");
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
    func_0804acb0("VisitCache::Dump()\n");
    unsigned *row = unk_00;
    for (int i = 0; i != unk_10; i++) {
        func_0804ac50(" [%3d] ", i);
        for (int j = 0; j != unk_04; j++)
            func_0804ac50(" %08x", row[j]);
        func_0804ac50("\n");
        row += unk_04;
    }
    func_0804acb0("VisitCache:: %d records\n", unk_10);
}

static inline bool testPathBit(VisitCache *vc, int rec, int bit)
{
    unsigned *row = vc->unk_00 + rec * vc->unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

void dumpPathKey(Program *p, int key)
{
    func_0804acb0("dumping path key '%d'\n", key);
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (testPathBit(&p->unk_bc, key, s->unk_2c0))
            func_0804acb0("    %d: %s\n", s->func_0806d674());
    }
    func_0804acb0(" done dumping path key '%d'\n", key);
}
