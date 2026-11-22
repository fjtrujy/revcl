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
bool emulateAndCollectRegisterUses(Program *p, instStream *s);
bool initBackTrace(Program *p);
bool disposeBackTrace(Program *p);
bool doBackTrace(Program *p, instStream *s, RegPoolEntry *reg, int part, int depth, int mode, bool hot);
bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b);

bool rebuildFlowDependencies(Program *p, bool b)
{
    dependencyBuildCount++;
    if (!flowReset(p)) {
        reportError("failed to reset cpu\n");
        return false;
    }
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!s->flowReset()) {
            reportError("failed to flow reset a stream\n");
            return false;
        }
    }
    if (!recalcFlowUIDs(p)) {
        reportError("flowMon::flowReset: failed to recalculate flow UIDs\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!resetProcessorThroughput(p)) {
            reportError("flowInit - failed for stream %s\n", s->streamName());
            return false;
        }
        if (!emulateAndCollectRegisterUses(p, s)) {
            reportError("build dependency - failed to %s stream..\n", s->getCodeModeName());
            return false;
        }
        if (!s->buildForwardDependencies(b, true)) {
            reportError("failed to build memory dependencies for ist\n");
            return false;
        }
        if (!s->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    if (!initBackTrace(p)) {
        reportError("flowMon:: failed to init back trace\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        for (regUse *ru = s->getWorkTable()->registerUses.head; ru; ru = ru->next) {
            RegPoolEntry *reg = ru->registerEntry;
            if (!reg) {
                reportError("found a reg ref with no reg!\n");
                return false;
            }
            for (int part = 0; part != reg->partCount; part++) {
                bool hot = false;
                int mode = 0;
                if ((ru->externalInputMask >> part) & 1) {
                    char bm = ru->externalBlockMode;
                    if (bm == -1) {
                        reportError("regref doesn't have block mode set\n");
                        return false;
                    }
                    mode = (bm != 0) + 1;
                }
                if (ru->outputMask & (1 << part) & reg->tracksHotOutputPaths)
                    hot = true;
                if (!hot && !mode)
                    continue;
                int depth;
                if (s->getCodeMode() == 3)
                    depth = ru->slots[part].timingValues[2];
                else {
                    depth = 30000;
                    if (mode == 1)
                        mode = 2;
                }
                if (!doBackTrace(p, s, reg, part, depth, mode, hot)) {
                    reportError("failed to back trace a register!\n");
                    return false;
                }
                if (p->resolvedStreams) {
                    for (instStream *t = p->resolvedStreams; t; t = t->nextResolvedStream) {
                        if (!int_buildTpairs(p, reg, part, t, s)) {
                            reportError("failed to build the tpairs\n");
                            return false;
                        }
                    }
                }
            }
        }
    }
    if (!disposeBackTrace(p)) {
        reportError("faield to dispose of the backtrace info\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!s->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}

bool gatherAllStreamStatistics(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!s->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}
