#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"
#include "ol_s_inline.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int insertStreamRegisterCaps(Program *p, instStream *s);
void initializeScheduledCycleTimes(workTable *wt);
bool removeNops(Program *p, instStream *s);
int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask);
bool doCap(int line, int pipe, int mask);
RegPool *GetRP(unsigned t);
bool resetProcessorThroughput(Program *p);
int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
void splitCtime(workTable *wt, int l, int m);


bool prepareExternalSchedulingForProgram(Program *p)
{
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream)
        s->externalSchedulingPrepared = s->usesFreeRawLayout() ? 0 : 1;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (s->externalSchedulingPrepared)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            reportError("failed to prep external info for stream %s\n", s->streamName());
            return false;
        }
        if (!removeNops(p, s)) {
            reportError("failed to sred a table from %s\n", s->streamName());
            return false;
        }
        s->externalSchedulingPrepared = 1;
    }
    return true;
}

bool removeNops(Program *p, instStream *s)
{
    if (!s->workTablePtr) {
        reportError("work table not set in %s\n", s->streamName());
        exitAfterCleanup(-1);
    }
    workTable *wt = s->workTablePtr;
    if (!wt) {
        reportError("no work table \n");
        return false;
    }
    int len = wt->getRangeEnd() - wt->getRangeStart();
    if (len == 0)
        return true;
    if (wt->externalMinimumLength < 0) {
        reportError("the minimal length is -ve for ext\n");
        return false;
    }
    if (len < wt->externalMinimumLength) {
        reportError("table is lready too short (%d<%d)\n", len, wt->externalMinimumLength);
        return false;
    }
    RegPool *hw = GetRP(10);
    if (!hw) {
        reportError("failed to get hw register pool - maybe this isn't vpu?\n");
        return false;
    }
    initializeScheduledCycleTimes(wt);
    if (!resetProcessorThroughput(p)) {
        reportError("failed to reset for emu\n");
        return false;
    }
    bool lastOk = true;
    if (!s->branchType) {
        if (s->endType != 1)
            lastOk = false;
    } else {
        lastOk = false;
    }
    int curTick = 0;
    int logLen = wt->getRawOrSyntheticLogicalLength();
    ol_s *inflight[1024];
    for (int rl = wt->getRangeStart(); rl != wt->getRangeEnd(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)wt)->getIE_ck(rl);
        if (row->skip) {
            reportError("skip already set for line\n");
            return false;
        }
        if (!row->countEntries()) {
            int tNow = row->cycleTime;
            int n = p->proc.gatherInflight(inflight);
            if (n < 0) {
                reportError("error gathering inflight data\n");
                return false;
            }
            int m = wt->getRangeEnd();
            int k;
            for (k = 0; k != n; k++) {
                ol_s *ol = inflight[k];
                Instr *in = ol->instruction;
                Operand *op = operandAtLocation(ol);
                RegPoolEntry *reg = op->assignedRegister;
                int part = ol->componentIndex;
                int lat = ol->cycleOffset;
                if (reg->tracksHotOutputPaths) {
                    int tv = in->baseIssueSpacing;
                    if (tv <= 6)
                        exitWithError("bad throught value in register\n");
                    int thr = lat;
                    if (reg->registerIndex == 0x34)
                        thr--;
                    else if (reg->registerIndex != 0x32)
                        exitWithError("non p-q has throughput?\n");
                    int l = rl + 1;
                    wtRow *r = row;
                    while (r++, l < wt->getRangeEnd()) {
                        int t = r->cycleTime;
                        if (t > tNow + thr)
                            break;
                        if (r->outputMaskFor(reg, 0xff)) {
                            if (t != tNow + thr) {
                                reportError("unexpected thruput stall detected\n");
                                return false;
                            }
                            if (l < m)
                                m = l;
                            break;
                        }
                        l++;
                    }
                }
                for (ol_s *u = op->parts[part].operandLocations.firstInternalLocation(); u && !u->isExt(); u = u->next) {
                    Instr *ui = u->instruction;
                    if (operandAtLocation(u)->externalBlockModeFlag && op->outputLatency > 1) {
                        int ul = ui->position.linearRow;
                        int tu = wt->ctime(ul);
                        if (tu <= tNow) {
                            reportError("user is before now .. tUser=%d tNow = %d\n", tu, tNow);
                            return false;
                        }
                        if (tNow + lat > tu) {
                            reportError("found an unexpected stall\n");
                            return false;
                        }
                        if (tNow + lat == tu && ul < m)
                            m = ul;
                    }
                }
            }
            bool ok = true;
            if (rl == wt->getRangeEnd() - 1 && !lastOk)
                ok = false;
            for (k = 0; k != n && ok; k++) {
                ol_s *ol = inflight[k];
                Operand *op = operandAtLocation(ol);
                int part = ol->componentIndex;
                int lat = ol->cycleOffset;
                ol_s *u = op->parts[part].operandLocations.firstInternalLocation();
                if (u) {
                    Instr *ui = u->instruction;
                    Operand *uo = operandAtLocation(u);
                    int ul = ui->position.linearRow;
                    int tu = wt->ctime(ul) - 1;
                    if (!op->externalBlockModeFlag && !uo->externalBlockModeFlag && ul < m && tNow + lat > tu)
                        ok = false;
                }
            }
            if (ok && rl < wt->getRangeEnd() - 1 && rl + 1 < m && row[1].getMinimumExtraLatencyAge() == 1)
                ok = false;
            if (m < wt->getRangeEnd()) {
                int tm = wt->ctime(m);
                wtRow *r = row + 1;
                for (int l = rl + 1; l < m && ok; l++, r++) {
                    int base = r->cycleTime - 1;
                    for (int pp = 0; pp != 2; pp++) {
                        Instr *in = r->getEntry(pp);
                        if (!in)
                            continue;
                        for (int i = 0; i != in->operandCount; i++) {
                            Operand *o = &in->operands[i];
                            RegPoolEntry *w = registerOperand(o, 2);
                            if (o->reductionHazardMask)
                                ok = false;
                            if (w && !o->externalBlockModeFlag) {
                                int t = base + o->outputLatency;
                                if (t < tm) {
                                    int l2 = m;
                                    wtRow *r2 = (wtRow *)((cls_unk30 *)wt)->getIE_ck(m);
                                    while (l2 < wt->getRangeEnd() && r2->cycleTime < t) {
                                        r2++;
                                        l2++;
                                    }
                                    if (l2 < wt->getRangeEnd()) {
                                        if (r2->cycleTime == t) {
                                            if (r2->inputMaskFor(w, 0xff))
                                                ok = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (ok) {
                if (rl + 1 < m) {
                    wtRow *r = row + 1;
                    int l = rl + 1;
                    do {
                        int base = r->cycleTime - 1;
                        for (int pp = 0; pp != 2; pp++) {
                            Instr *in = GetI((Instr **)r, pp);
                            if (in) {
                                short e = in->externalInputConstraint;
                                if (e < in->externalInputPairedConstraint)
                                    e = in->externalInputPairedConstraint;
                                if (e > base) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                        l++;
                        r++;
                    } while (l < m && ok);
                }
                if (ok) {
                    if (m >= wt->getRangeEnd()) {
                        int last = wt->getRawOrSyntheticLogicalLength() - 1;
                        for (int l = wt->getRangeStart(); l < m && ok; l++) {
                            wtRow *r = (wtRow *)((cls_unk30 *)wt)->getIE_ck(l);
                            int dist = last - row->cycleTime;
                            if (dist < 0)
                                exitWithError("distance to end appears to be less than zero\n");
                            for (int pp = 0; pp != 2; pp++) {
                                Instr *in = GetI((Instr **)r, pp);
                                if (in) {
                                    short e = in->externalOutputConstraint;
                                    if (e < in->externalOutputPairedConstraint)
                                        e = in->externalOutputPairedConstraint;
                                    if (e < dist) {
                                        ok = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (ok) {
                        if (m >= wt->getRangeEnd()) {
                            int last = wt->getRawOrSyntheticLogicalLength() - 1;
                            if (last < wt->externalMinimumLength)
                                ok = false;
                            if (s->endType != 1 && last <= 1)
                                ok = false;
                        }
                        if (ok) {
                            row->skip = 1;
                            splitCtime(wt, rl, m);
                            if (m >= wt->getRangeEnd())
                                wt->declaredRawLength = --logLen;
                        }
                    }
                }
            }
        }
        int d = emuInst(p, row, rl, false, false);
        if (d < 0) {
            reportError("failed to emu an ientry\n");
            return false;
        }
        curTick += d;
        if (curTick != row->cycleTime + 1) {
            reportError("unexpected time !.. currTick=%d cTime=%d\n", curTick, row->cycleTime);
            return false;
        }
        if (curTick != p->emulatedCycleCount) {
            reportError("currTick doesn't match cpu\n");
            return false;
        }
    }
    if (wt->getRawOrSyntheticLogicalLength() != p->emulatedCycleCount) {
        reportError("at end of table logical len not correct\n");
        return false;
    }
    return true;
}

void initializeScheduledCycleTimes(workTable *wt)
{
    wtRow *row = wt->rows + wt->rangeStart;
    for (int l = wt->rangeStart; l != wt->rangeEnd; l++, row++) {
        int rel = l - wt->rangeStart;
        for (int p = 0; p != 2; p++) {
            Instr *in = GetI((Instr **)row, p);
            if (in && in->scheduledCycle != rel)
                exitWithError("l=%d exec time mismatch in instruction %d!=%d\n",
                              l, in->scheduledCycle, rel);
        }
        short *time = &row->cycleTime;
        *time = l - wt->rangeStart;
    }
}

void splitCtime(workTable *wt, int l, int m)
{
    if (l < wt->rangeStart || l >= wt->rangeEnd)
        exitWithError("nop line is bad\n");
    if (m <= l)
        exitWithError("m<l for splitCtime()\n");
    if (m > wt->rangeEnd)
        m = wt->rangeEnd;
    wtRow *row = &wt->rows[l];
    for (; l < m; l++, row++)
        row->cycleTime--;
}

int getCtime(workTable *wt, int l)
{
    if (l < wt->rangeStart || l >= wt->rangeEnd)
        exitWithError("nop line is bad\n");
    return wt->rows[l].cycleTime;
}

