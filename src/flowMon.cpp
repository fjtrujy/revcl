#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "RegPool.h"
#include "os.h"
#include "subCache.h"
#include "VisitCache.h"

int buildDepCount_080b33a4 = 0;

bool flowReset(Program *p);
bool recalcFlowUIDs(Program *p);
bool func_0806c0f8(Program *p);
bool func_080642ec(Program *p, instStream *s);
bool initBackTrace(Program *p);
bool disposeBackTrace(Program *p);
bool doBackTrace(Program *p, instStream *s, RegPoolEntry *reg, int part, int depth, int mode, bool hot);
bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b);

bool func_08060018(Program *p, bool b)
{
    buildDepCount_080b33a4++;
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
        if (!func_0806c0f8(p)) {
            reportError("flowInit - failed for stream %s\n", s->streamName());
            return false;
        }
        if (!func_080642ec(p, s)) {
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
        for (regUse *ru = s->getWorkTable()->unk_14.head; ru; ru = ru->unk_04) {
            RegPoolEntry *reg = ru->unk_08;
            if (!reg) {
                reportError("found a reg ref with no reg!\n");
                return false;
            }
            for (int part = 0; part != reg->partCount; part++) {
                bool hot = false;
                int mode = 0;
                if ((ru->unk_0c >> part) & 1) {
                    char bm = ru->unk_15;
                    if (bm == -1) {
                        reportError("regref doesn't have block mode set\n");
                        return false;
                    }
                    mode = (bm != 0) + 1;
                }
                if (ru->unk_0f & (1 << part) & reg->unk_2c)
                    hot = true;
                if (!hot && !mode)
                    continue;
                int depth;
                if (s->getCodeMode() == 3)
                    depth = ru->unk_20[part].unk_3c[2];
                else {
                    depth = 30000;
                    if (mode == 1)
                        mode = 2;
                }
                if (!doBackTrace(p, s, reg, part, depth, mode, hot)) {
                    reportError("failed to back trace a register!\n");
                    return false;
                }
                if (p->unk_e0) {
                    for (instStream *t = p->unk_e0; t; t = t->unk_258) {
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

bool func_080603bc(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!s->gatherStats()) {
            reportError("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}
