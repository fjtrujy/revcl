#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "RegPool.h"
#include "os.h"
#include "subCache.h"
#include "VisitCache.h"

int dependencyBuildCount = 0;

bool flowReset(Program *p);
bool recalcFlowUIDs(Program *p);
bool resetProcessorThroughput(Program *p);
bool emulateAndCollectRegisterUses(Program *p, InstructionStream *s);
bool initBackTrace(Program *p);
bool disposeBackTrace(Program *p);
bool doBackTrace(Program *p, InstructionStream *s, RegisterEntry *reg, int part, int depth, int mode, bool hot);
bool buildCrossStreamThroughputPairs(Program *p, RegisterEntry *reg, int part, InstructionStream *t, void *b);

bool rebuildFlowDependencies(Program *program, bool forceDependencyChecks)
{
    dependencyBuildCount++;
    if (!flowReset(program)) {
        reportError("failed to reset cpu\n");
        return false;
    }
    InstructionStream *streamCursor;
    for (streamCursor = program->streamHead; streamCursor; streamCursor = streamCursor->nextStream) {
        if (!streamCursor->flowReset()) {
            reportError("failed to flow reset a stream\n");
            return false;
        }
    }
    if (!recalcFlowUIDs(program)) {
        reportError("flowMon::flowReset: failed to recalculate flow UIDs\n");
        return false;
    }
    for (streamCursor = program->streamHead; streamCursor; streamCursor = streamCursor->nextStream) {
        if (!resetProcessorThroughput(program)) {
            reportError("flowInit - failed for stream %s\n", streamCursor->streamName());
            return false;
        }
        if (!emulateAndCollectRegisterUses(program, streamCursor)) {
            reportError("build dependency - failed to %s stream..\n", streamCursor->getCodeModeName());
            return false;
        }
        if (!streamCursor->buildForwardDependencies(forceDependencyChecks, true)) {
            reportError("failed to build memory dependencies for ist\n");
            return false;
        }
        if (!streamCursor->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    if (!initBackTrace(program)) {
        reportError("flowMon:: failed to init back trace\n");
        return false;
    }
    for (streamCursor = program->streamHead; streamCursor; streamCursor = streamCursor->nextStream) {
        for (RegisterUse *registerUse = streamCursor->getWorkTable()->registerUses.head; registerUse; registerUse = registerUse->next) {
            RegisterEntry *registerEntry = registerUse->registerEntry;
            if (!registerEntry) {
                reportError("found a reg ref with no reg!\n");
                return false;
            }
            for (int part = 0; part != registerEntry->partCount; part++) {
                bool hot = false;
                int mode = 0;
                if ((registerUse->externalInputMask >> part) & 1) {
                    char externalBlockMode = registerUse->externalBlockMode;
                    if (externalBlockMode == -1) {
                        reportError("regref doesn't have block mode set\n");
                        return false;
                    }
                    mode = (externalBlockMode != 0) + 1;
                }
                if (registerUse->outputMask & (1 << part) & registerEntry->tracksHotOutputPaths)
                    hot = true;
                if (!hot && !mode)
                    continue;
                int depth;
                if (streamCursor->getCodeMode() == 3)
                    depth = registerUse->slots[part].timingValues[2];
                else {
                    depth = 30000;
                    if (mode == 1)
                        mode = 2;
                }
                if (!doBackTrace(program, streamCursor, registerEntry, part, depth, mode, hot)) {
                    reportError("failed to back trace a register!\n");
                    return false;
                }
                if (program->resolvedStreams) {
                    for (InstructionStream *resolvedStream = program->resolvedStreams; resolvedStream; resolvedStream = resolvedStream->nextResolvedStream) {
                        if (!buildCrossStreamThroughputPairs(program, registerEntry, part, resolvedStream, streamCursor)) {
                            reportError("failed to build the tpairs\n");
                            return false;
                        }
                    }
                }
            }
        }
    }
    if (!disposeBackTrace(program)) {
        reportError("faield to dispose of the backtrace info\n");
        return false;
    }
    for (streamCursor = program->streamHead; streamCursor; streamCursor = streamCursor->nextStream) {
        if (!streamCursor->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}

bool gatherAllStreamStatistics(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}
