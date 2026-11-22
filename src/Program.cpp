#include "Program.h"
#include "Processor.h"
#include "instStream.h"
#include "os.h"
#include "decodedLine.h"

void disposeBackTrace(Program *p);
void dumpProcessorRegisterPools(Program *p);
bool parseReset(Program *p);
const char *linkTypeName(int type);
bool equalExactStrings(const char *a, const char *b);
bool emitReset(Program *p);
char *emitStats(Program *p, char *buf);
char *emitFileStart(Program *p, char *buf);
char *emitFileEnd(Program *p, char *buf);
const char *getOptimiserName(int id);

Program::Program(bool symbolic)
{
    symbolicOutput = symbolic;
    streamTail = 0;
    streamHead = 0;
    streamCount = 0;
    streamsById = 0;
}

Program::~Program()
{
    dispose();
}

bool Program::Init(CpuDef *cpu, const char *uniqueName, bool uniqueAll)
{
    if (!dispose()) {
        reportError("flowMon::Init() - failed to dispose\n");
        return false;
    }
    if (!cpu) {
        reportError("flowMon::init - bad cpu type\n");
        dispose();
        return false;
    }
    if (!proc.setUniqueName(uniqueName)) {
        reportError("flowMon::Init - failed to set name\n");
        dispose();
        return false;
    }
    proc.uniqueAll = uniqueAll;
    if (!proc.Init(cpu)) {
        reportError("flowMon::Init failed to init cpu object\n");
        return false;
    }
    return true;
}

bool Program::dispose()
{
    instStream *stream = streamHead;
    while (stream) {
        instStream *next = stream->nextStream;
        stream->reset();
        delete stream;
        stream = next;
    }
    streamTail = 0;
    streamHead = 0;
    streamCount = 0;
    if (streamsById)
        delete[] streamsById;
    streamsById = 0;
    disposeBackTrace(this);
    uniqueNamePrefixes.clearNodes();
    return true;
}


bool Program::addStream(instStream *stream)
{
    if (!stream)
        return false;
    if (stream->owningProgram) {
        reportError("addistream - owner already set\n");
        return false;
    }
    if (streamHead) {
        streamTail->nextStream = stream;
        stream->previousStream = streamTail;
        stream->nextStream = 0;
        streamTail = stream;
    } else {
        streamHead = stream;
        streamTail = stream;
        stream->nextStream = 0;
        stream->previousStream = 0;
    }
    stream->owningProgram = this;
    streamCount++;
    return true;
}

bool Program::insertStreamAfter(instStream *after, instStream *stream)
{
    if (!stream)
        return false;
    if (stream->owningProgram) {
        reportError("addistream - owner already set\n");
        return false;
    }
    stream->previousStream = after;
    if (after) {
        stream->nextStream = after->nextStream;
        if (after->nextStream)
            after->nextStream->previousStream = stream;
        else
            streamTail = stream;
        after->nextStream = stream;
    } else {
        stream->nextStream = streamHead;
        if (streamHead)
            streamHead->previousStream = stream;
        streamHead = stream;
    }
    stream->owningProgram = this;
    streamCount++;
    return true;
}

bool Program::removeStream(instStream *stream)
{
    if (stream->owningProgram != this) {
        reportError("didn't have owner! or not us!\n");
        return false;
    }
    if (stream->nextStream)
        stream->nextStream->previousStream = stream->previousStream;
    else
        streamTail = stream->previousStream;
    if (stream->previousStream)
        stream->previousStream->nextStream = stream->nextStream;
    else
        streamHead = stream->nextStream;
    stream->owningProgram = 0;
    stream->nextStream = 0;
    stream->previousStream = 0;
    streamCount--;
    return true;
}

void dumpProgramStreamsAndRegisters(Program *program, int dumpFlags)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream)
        stream->dump(dumpFlags);
    dumpProcessorRegisterPools(program);
}

instStream *Program::findStreamById(int streamId)
{
    instStream *stream;
    for (stream = streamHead; stream && stream->streamId != streamId; stream = stream->nextStream)
        ;
    return stream;
}

instStream *Program::findStream(const char *name)
{
    if (!name)
        return 0;
    instStream *stream;
    for (stream = streamHead; stream && !stream->labels.findByNameIgnoreCase(name); stream = stream->nextStream)
        ;
    return stream;
}

bool Program::retarget(instStream *from, instStream *stream)
{
    if (from == stream)
        return true;
    linkNode *next;
    for (linkNode *incomingLink = from->inputLinks.head; incomingLink; incomingLink = next) {
        next = incomingLink->next;
        instStream *predecessorStream = incomingLink->otherStream;
        if (!predecessorStream) {
            reportError("null ist pointer found on ist list\n");
            return false;
        }
        linkNode *outgoingLink = predecessorStream->outputLinks.findLinkToStream(from, incomingLink->linkType);
        if (!outgoingLink) {
            reportError("Retarget:: failed to find matching output\n");
            return false;
        }
        outgoingLink->otherStream = stream;
        if (!from->inputLinks.unlink(incomingLink)) {
            reportError("Retarget::failed to unlink input from old\n");
            return false;
        }
        if (!stream->inputLinks.relink(incomingLink)) {
            reportError("Retarget::failed to relink input to new\n");
            return false;
        }
        switch (outgoingLink->linkType) {
        case 2:
        case 4:
        case 5:
            reportWarning("flowMon::can't reliably retarget a branch-to-register instruction\n");
            break;
        case 0:
            break;
        case 1:
        case 3: {
            Instr *branchInstruction = predecessorStream->getBranch();
            if (branchInstruction && !branchInstruction->changeBranchImmediate(stream->streamName())) {
                reportError("failed to change branch immediate\n");
                exitAfterCleanup(-1);
            }
            break;
        }
        default:
            const char *typeName = linkTypeName(outgoingLink->linkType);
            reportError("flowMon::Retarget - unexpected output type %d %s\n",
                          outgoingLink->linkType, typeName);
            reportError("error retargeting %s->%s\n",
                          from->streamName(), stream->streamName());
            return false;
        }
    }
    return true;
}

bool Program::decodeAndSplitStreams(Parser *parser, int buildType, bool newSyntax)
{
    if (!parseReset(this)) {
        reportError("flow::failed to reset cpu\n");
        return false;
    }
    int count = 0;
    DecodedLine *decodedLines = decodeSourceLines(this, parser, buildType, newSyntax, &count);
    if (!decodedLines) {
        reportError("flow: failed to build it from parse\n");
        return false;
    }
    if (count <= 0) {
        reportError("<=0 number of decoded line entries from source??\n");
        delete decodedLines;
        return false;
    }
    bool splitSucceeded = splitParserStream(this, decodedLines, count);
    if (!splitSucceeded) {
        for (int lineIndex = 0; lineIndex != count; lineIndex++)
            decodedLines[lineIndex].releaseContents();
        reportError("flow: failed to split parser stream\n");
    }
    delete[] decodedLines;
    return splitSucceeded;
}

bool patchAllStreamLatencies(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->patchLatencies())
            reportError("flowMon::PatchLatencies - failed for '%s'\n", stream->streamName());
    }
    return true;
}

char *emitProgram(Program *program, char *out, int comments)
{
    instStream *stream;
    int total = 0;
    if (!out) {
        reportError("flowMon::Emit - null output pointer\n");
        return 0;
    }
    char *outputCursor = out;
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (stream->optimizationRecordCount) {
            outputCursor += formatIntoBuffer(outputCursor, "; === __LP__ %s:\n", stream->streamName());
            for (int optimizerIndex = 0; optimizerIndex != stream->optimizationRecordCount; optimizerIndex++) {
                if (stream->optimizationRecords[optimizerIndex].instructionCount > 0) {
                    outputCursor += formatIntoBuffer(outputCursor, "; === %16s : optimal=%d clid=%d mlid=%d size=(%d) ",
                                         getOptimiserName(optimizerIndex), stream->optimizationRecords[optimizerIndex].optimalLength, stream->optimizationRecords[optimizerIndex].candidateLoopId,
                                         stream->optimizationRecords[optimizerIndex].mainLoopId, stream->optimizationRecords[optimizerIndex].instructionCount);
                    outputCursor += formatIntoBuffer(outputCursor, "\n");
                }
            }
        }
    }
    outputCursor += formatIntoBuffer(outputCursor, "; =================================================\n");
    outputCursor += formatIntoBuffer(outputCursor, "; flowMon::Emit() vcl 1.4beta7 produced this code:\n");
    emitReset(program);
    outputCursor = emitFileStart(program, outputCursor);
    if (!outputCursor) {
        reportError("failed to emit the start of the source file\n");
        return 0;
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        stream->dump(0);
        int emittedInstructionCount = 0, m = 0;
        stream->emissionStartIndex = total;
        outputCursor = stream->emit(outputCursor, comments, &emittedInstructionCount, &m);
        if (!outputCursor) {
            reportError("flowMon : failed to write a stream\n");
            return 0;
        }
        total += emittedInstructionCount;
    }
    outputCursor = emitFileEnd(program, outputCursor);
    if (!outputCursor) {
        reportError("failed to emit end of file\n");
        return 0;
    }
    outputCursor += formatIntoBuffer(outputCursor, ";\tiCount=%d\n", total);
    outputCursor = emitStats(program, outputCursor);
    if (!outputCursor) {
        reportError("failed to emit cpu stats\n");
        return 0;
    }
    return outputCursor;
}

bool setUniqueName(Program *program, const char *name)
{
    return program->proc.setUniqueName(name);
}

const char *getUniqueName(Program *program)
{
    return program->proc.getUniqueName();
}

char *emitLabels(Program *program, char *buf)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        buf = stream->emitLabels(buf);
        if (!buf) {
            reportError("failed to emit user labels for '%s'\n", stream->streamName());
            return 0;
        }
    }
    return buf;
}

bool isReferencedLabel(Program *program, char *name)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (stream->startType == 2) {
            linkNode *outputLink = stream->outputLinks.head;
            if (outputLink) {
                linkNode *next = outputLink->next;
                if (next) {
                    reportError("found entry point with more than one exit\n");
                    exitAfterCleanup(-1);
                }
                instStream *targetStream = outputLink->otherStream;
                if (!targetStream) {
                    reportError("no stream for ir\n");
                    exitAfterCleanup(-1);
                }
                if (targetStream->labels.findByExactName(name))
                    return true;
            }
        }
        if (stream->getBranch()) {
            char *tgt = stream->getBranchTargetImmediate();
            if (tgt && equalExactStrings(tgt, name))
                return true;
        }
    }
    return program->uniqueNamePrefixes.findByExactName(name) != 0;
}

bool streamExportsSymbol(Program *program, instStream *stream)
{
    mList *streamLabels = &stream->labels;
    for (mNode *labelNode = streamLabels->head; labelNode; labelNode = labelNode->next) {
        if (program->proc.exportedSymbols.findByExactName(labelNode->name))
            return true;
    }
    return false;
}

bool isExportedSymbol(Program *program, char *name)
{
    return program->proc.exportedSymbols.findByExactName(name) != 0;
}

bool recalcFlowUIDs(Program *program)
{
    instStream *stream = program->streamHead;
    int streamIndex = 0;
    program->flowIdCount = -1;
    for (; stream; stream = stream->nextStream) {
        if (!stream->getCodeMode()) {
            reportError("recalcFlowUIDs - found bad code mode at %s\n", stream->streamName());
            return false;
        }
        stream->streamId = streamIndex++;
    }
    if (streamIndex != program->streamCount) {
        reportError("recalcFlowUIDs - there was a hole somewhere?\n");
        return false;
    }
    program->flowIdCount = streamIndex;
    int size = program->flowIdCount + 1;
    if (program->streamsById) {
        delete program->streamsById;
        program->streamsById = 0;
    }
    program->streamsById = new instStream *[size];
    if (!program->streamsById) {
        reportError("failed to get a new linear table\n");
        return false;
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream)
        program->streamsById[stream->streamId] = stream;
    return true;
}

bool resetProgramAllocationsForFirstPass(Program *program)
{
    if (!program->proc.clearAllRegisterAssignmentsFirstPass()) {
        reportError("failed to alloc reset the cpu\n");
        return false;
    }
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *table = stream->getTable();
        if (!table) {
            reportError("failed to get work table\n");
            return false;
        }
        table->allocationMode = 1;
    }
    return true;
}

bool resetProgramAllocationsForSecondPass(Program *program)
{
    if (!program->proc.clearAllRegisterAssignmentsSecondPass()) {
        reportError("failed to alloc reset the cpu\n");
        return false;
    }
    return true;
}

static inline void printNamedLink(const char *fmt, linkNode *link, const char *name)
{
    int other = link->otherStream->streamId;
    const char *type = linkTypeName(link->linkType);
    reportInfo(fmt, type, other, name);
}

static inline bool isEntryStream(instStream *stream) { return stream->startType == 2; }
static inline bool isExitStream(instStream *stream) { return stream->endType == 3; }

void dumpFlow(Program *program)
{
    instStream *stream = program->streamHead;
    reportInfo("dumpFlow-------------------\n");
    for (; stream; stream = stream->nextStream) {
        const char *tag = "";
        if (isEntryStream(stream))
            tag = " ---ENTRY";
        if (isExitStream(stream))
            tag = " ---EXIT";
        bool fallThrough = false;
        int streamId = stream->streamId;
        reportInfo("[%5d] %s %s   <--\n", streamId, stream->streamName(), tag);
        linkNode *link;
        for (link = stream->inputLinks.head; link; link = link->next) {
            if (link->linkType) {
                const char *name = link->otherStream->streamName();
                printNamedLink("       <--[(%s) [%5d]%s]\n", link, name);
            }
        }
        for (link = stream->outputLinks.head; link; link = link->next) {
            if (link->linkType) {
                const char *name = link->otherStream->streamName();
                printNamedLink("     --> %s [%5d]%s\n", link, name);
            } else
                fallThrough = true;
        }
        if (fallThrough) {
            reportInfo("    |\n");
            reportInfo("    V\n");
        } else
            reportInfo("\n");
    }
    reportInfo("dumpFlow - done\n");
}
