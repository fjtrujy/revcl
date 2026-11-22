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
        instStream *followingStream = stream->nextStream;
        if (followingStream && !followingStream->containsBarrier && !stream->mergedBarrierFlag) {
        linkList *out = &stream->outputLinks;
        if (out->count == 1 && followingStream->inputLinks.count == 1
            && stream->usesFreeRawLayout() && followingStream->usesFreeRawLayout()
            && stream->endType == 1 && followingStream->startType == 1
            && !stream->getBranch()
            && out->getStream(0) == followingStream
            && !followingStream->leadingLabelFlags && !stream->trailingLabelFlags
            && !streamExportsSymbol(program, followingStream)) {
            if (!appendStream(program, stream, followingStream)) {
                reportError("failed to append %s onto %s\n", followingStream->streamName(), stream->streamName());
                return 0;
            }
            if (!program->unlinkStreams(stream, followingStream, 0)) {
                reportError("failed to unlink streams\n");
                return 0;
            }
            if (out->count) {
                reportError("there's still out ist's after unlink\n");
                return 0;
            }
            linkNode *outgoingLink;
            while ((outgoingLink = followingStream->outputLinks.head) != 0) {
                instStream *destinationStream = outgoingLink->otherStream;
                int type = outgoingLink->linkType;
                if (!program->unlinkStreams(followingStream, destinationStream, type)) {
                    reportError("failed to unlink streams\n");
                    return 0;
                }
                if (!program->linkStreams(stream, destinationStream, type)) {
                    reportError("failed to link together streams\n");
                    return 0;
                }
            }
            if (!program->removeStream(followingStream)) {
                reportError("failed to unlink ist_n\n");
                return 0;
            }
            delete followingStream;
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
    int rowIndex;
    for (rowIndex = t2->getRangeStart(); rowIndex < t2->getRangeEnd(); dst++, rowIndex++) {
        wtRow *src = &t2->rows[rowIndex];
        memcpy(&t1->rows[dst], src, 14);
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *sourceInstruction = src->entries[entryIndex].instruction;
            if (sourceInstruction) {
                if (!UnlinkFromLinear((Block *)t2, sourceInstruction)) {
                    reportError("failed to remlinear\n");
                    return 0;
                }
                sourceInstruction->ownerTable = 0;
                sourceInstruction->setOwner((cls_unk30 *)t1);
            }
            src->entries[entryIndex].instruction = 0;
        }
    }
    for (rowIndex = t1->rangeStart; rowIndex < t1->rangeEnd; rowIndex++) {
        wtRow *workRow = &t1->rows[rowIndex];
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            Instr *destinationInstruction = workRow->entries[pipeIndex].instruction;
            if (destinationInstruction && !UnlinkFromLinear((Block *)t1, destinationInstruction)) {
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

bool appendStream(Program *program, instStream *stream, instStream *followingStream)
{
    if (!stream || !followingStream) {
        reportError("cat streams - passed a null pointer\n");
        return 0;
    }
    if (followingStream->leadingLabelFlags) {
        reportError("dbgTop set for ist_2 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    if (stream->trailingLabelFlags) {
        reportError("dbgEnd set for ist_1 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    workTable *t1 = stream->getTable();
    workTable *t2 = followingStream->getWorkTable();
    if (!catOnTo(t1, t2)) {
        reportError("failed to cat table2 onto table1\n");
        return 0;
    }
    int endType = followingStream->endType;
    stream->endType = endType;
    int branchType = followingStream->branchType;
    stream->branchType = branchType;
    stream->trailingLabelFlags = followingStream->trailingLabelFlags;
    stream->mergedBarrierFlag = followingStream->mergedBarrierFlag;
    return 1;
}

