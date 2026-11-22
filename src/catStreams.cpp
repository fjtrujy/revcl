#include <string.h>
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "os.h"

bool UnlinkFromLinear(Block *blk, Instr *n);
bool streamExportsSymbol(Program *p, instStream *s);
bool appendStream(Program *p, instStream *a, instStream *b);

static inline void dumpRawStream(instStream *stream)
{
    stream->dump(0);
}

bool concatenateCompatibleStreams(Program *program)
{
    instStream *stream = program->streamHead;
    while (stream) {
        if (stream->getCodeMode() != 4 && !stream->disposeInputTable()) {
            reportError("failed to dispose input table\n");
            return 0;
        }
        if (stream->getCodeMode() == 3 && !stream->getRawSchedulingMode()) {
            reportError("bad raw mode\n");
            dumpRawStream(stream);
            return 0;
        }
        instStream *n = stream->nextStream;
        if (n && !n->containsBarrier && !stream->mergedBarrierFlag) {
        linkList *out = &stream->outputLinks;
        if (out->count == 1 && n->inputLinks.count == 1
            && stream->usesFreeRawLayout() && n->usesFreeRawLayout()
            && stream->endType == 1 && n->startType == 1
            && !stream->getBranch()
            && out->getStream(0) == n
            && !n->leadingLabelFlags && !stream->trailingLabelFlags
            && !streamExportsSymbol(program, n)) {
            if (!appendStream(program, stream, n)) {
                reportError("failed to append %s onto %s\n", n->streamName(), stream->streamName());
                return 0;
            }
            if (!program->unlinkStreams(stream, n, 0)) {
                reportError("failed to unlink streams\n");
                return 0;
            }
            if (out->count) {
                reportError("there's still out ist's after unlink\n");
                return 0;
            }
            linkNode *l;
            while ((l = n->outputLinks.head) != 0) {
                instStream *to = l->otherStream;
                int type = l->linkType;
                if (!program->unlinkStreams(n, to, type)) {
                    reportError("failed to unlink streams\n");
                    return 0;
                }
                if (!program->linkStreams(stream, to, type)) {
                    reportError("failed to link together streams\n");
                    return 0;
                }
            }
            if (!program->removeStream(n)) {
                reportError("failed to unlink ist_n\n");
                return 0;
            }
            delete n;
            continue;
        }
        }
        stream = stream->nextStream;
    }
    return 1;
}

// workTable is passed where Block / cls_unk30 are expected (same class, not yet unified)
bool catOnTo(workTable *t1, workTable *t2)
{
    if (!t1 || !t2) {
        reportError("catOnTo - passed a null pointer\n");
        return 0;
    }
    if (!t1->rows || !t2->rows) {
        reportError("it_1 or it_2 one of them has no table\n");
        return 0;
    }
    int end = t1->getRangeEnd();
    int physicalLen = t2->getPhysicalLen();
    if (!t1->grow(end + physicalLen + 8)) {
        reportError("failed to grow table1\n");
        return 0;
    }
    int dst = t1->rangeEnd;
    int i;
    for (i = t2->getRangeStart(); i < t2->getRangeEnd(); dst++, i++) {
        wtRow *src = &t2->rows[i];
        memcpy(&t1->rows[dst], src, 14);
        for (int j = 0; j != 2; j++) {
            Instr *in = src->entries[j].instruction;
            if (in) {
                if (!UnlinkFromLinear((Block *)t2, in)) {
                    reportError("failed to remlinear\n");
                    return 0;
                }
                in->ownerTable = 0;
                in->setOwner((cls_unk30 *)t1);
            }
            src->entries[j].instruction = 0;
        }
    }
    for (i = t1->rangeStart; i < t1->rangeEnd; i++) {
        wtRow *workRow = &t1->rows[i];
        for (int j = 0; j != 2; j++) {
            Instr *in = workRow->entries[j].instruction;
            if (in && !UnlinkFromLinear((Block *)t1, in)) {
                reportError("failed to link in from it1\n");
                return 0;
            }
        }
    }
    t1->rangeEnd = dst;
    t2->rangeEnd = 0;
    t2->rangeStart = 0;
    if (!t1->prepRawTable()) {
        reportError("failed to prep it_1\n");
        return 0;
    }
    if (!t2->prepRawTable()) {
        reportError("failed to prep it_2 after removing insts\n");
        return 0;
    }
    return 1;
}

bool appendStream(Program *program, instStream *stream, instStream *b)
{
    if (!stream || !b) {
        reportError("cat streams - passed a null pointer\n");
        return 0;
    }
    if (b->leadingLabelFlags) {
        reportError("dbgTop set for ist_2 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    if (stream->trailingLabelFlags) {
        reportError("dbgEnd set for ist_1 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    workTable *t1 = stream->getTable();
    workTable *t2 = b->getWorkTable();
    if (!catOnTo(t1, t2)) {
        reportError("failed to cat table2 onto table1\n");
        return 0;
    }
    int endType = b->endType;
    stream->endType = endType;
    int branchType = b->branchType;
    stream->branchType = branchType;
    stream->trailingLabelFlags = b->trailingLabelFlags;
    stream->mergedBarrierFlag = b->mergedBarrierFlag;
    return 1;
}

