#pragma implementation "subCache.h"
#pragma implementation "VisitCache.h"
#pragma implementation "VisitCache_inline.h"
#include "Program.h"
#include "VisitCache_inline.h"
#include "instStream.h"
#include "workTable.h"
#include "RegPool.h"
#include "os.h"
#include "subCache.h"
#include "VisitCache.h"

int buildDepCount_080b33a4;

bool flowReset(Program *p);
bool recalcFlowUIDs(Program *p);
bool func_0806c0f8(Program *p);
bool func_080642ec(Program *p, instStream *s);
bool initBackTrace(Program *p);
bool disposeBackTrace(Program *p);
bool func_08062038(Program *p, instStream *s, RegPoolEntry *reg, int part, int depth, int mode, bool hot);
bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b);

bool func_08060018(Program *p, bool b)
{
    buildDepCount_080b33a4++;
    if (!flowReset(p)) {
        func_0804ad38("failed to reset cpu\n");
        return false;
    }
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!s->flowReset()) {
            func_0804ad38("failed to flow reset a stream\n");
            return false;
        }
    }
    if (!recalcFlowUIDs(p)) {
        func_0804ad38("flowMon::flowReset: failed to recalculate flow UIDs\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!func_0806c0f8(p)) {
            func_0804ad38("flowInit - failed for stream %s\n", s->func_0806d674());
            return false;
        }
        if (!func_080642ec(p, s)) {
            func_0804ad38("build dependency - failed to %s stream..\n", s->getCodeModeName());
            return false;
        }
        if (!s->func_0806563c(b, true)) {
            func_0804ad38("failed to build memory dependencies for ist\n");
            return false;
        }
        if (!s->func_0805e4bc()) {
            func_0804ad38("failed to gather stats for the istream\n");
            return false;
        }
    }
    if (!initBackTrace(p)) {
        func_0804ad38("flowMon:: failed to init back trace\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        for (regUse *ru = s->getWorkTable()->unk_14.unk_04; ru; ru = ru->unk_04) {
            RegPoolEntry *reg = ru->unk_08;
            if (!reg) {
                func_0804ad38("found a reg ref with no reg!\n");
                return false;
            }
            for (int part = 0; part != reg->unk_28; part++) {
                bool hot = false;
                int mode = 0;
                if ((ru->unk_0c >> part) & 1) {
                    char bm = ru->unk_15;
                    if (bm == -1) {
                        func_0804ad38("regref doesn't have block mode set\n");
                        return false;
                    }
                    mode = (bm != 0) + 1;
                }
                if (ru->unk_0f & (1 << part) & reg->unk_2c)
                    hot = true;
                if (!hot && !mode)
                    continue;
                int depth;
                if (s->func_0806d908() == 3)
                    depth = ru->unk_20[part].unk_3c[2];
                else {
                    depth = 30000;
                    if (mode == 1)
                        mode = 2;
                }
                if (!func_08062038(p, s, reg, part, depth, mode, hot)) {
                    func_0804ad38("failed to back trace a register!\n");
                    return false;
                }
                for (instStream *t = p->unk_e0; t; t = t->unk_258) {
                    if (!int_buildTpairs(p, reg, part, t, s)) {
                        func_0804ad38("failed to build the tpairs\n");
                        return false;
                    }
                }
            }
        }
    }
    if (!disposeBackTrace(p)) {
        func_0804ad38("faield to dispose of the backtrace info\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!s->func_0805e4bc()) {
            func_0804ad38("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}

bool func_080603bc(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!s->func_0805e4bc()) {
            func_0804ad38("failed to gather stats for the istream\n");
            return false;
        }
    }
    return true;
}
