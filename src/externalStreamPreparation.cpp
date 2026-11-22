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


bool prepareExternalSchedulingForProgram(Program *program)
{
    instStream *stream;
    for (stream = program->streamHead; stream; stream = stream->nextStream)
        stream->externalSchedulingPrepared = stream->usesFreeRawLayout() ? 0 : 1;
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (stream->externalSchedulingPrepared)
            continue;
        if (!prepExtInfo(program, stream, 0)) {
            reportError("failed to prep external info for stream %s\n", stream->streamName());
            return false;
        }
        if (!removeNops(program, stream)) {
            reportError("failed to sred a table from %s\n", stream->streamName());
            return false;
        }
        stream->externalSchedulingPrepared = 1;
    }
    return true;
}

bool removeNops(Program *program, instStream *stream)
{
    if (!stream->workTablePtr) {
        reportError("work table not set in %s\n", stream->streamName());
        exitAfterCleanup(-1);
    }
    workTable *table = stream->workTablePtr;
    if (!table) {
        reportError("no work table \n");
        return false;
    }
    int len = table->getRangeEnd() - table->getRangeStart();
    if (len == 0)
        return true;
    if (table->externalMinimumLength < 0) {
        reportError("the minimal length is -ve for ext\n");
        return false;
    }
    if (len < table->externalMinimumLength) {
        reportError("table is lready too short (%d<%d)\n", len, table->externalMinimumLength);
        return false;
    }
    RegPool *hardwarePool = GetRP(10);
    if (!hardwarePool) {
        reportError("failed to get hw register pool - maybe this isn't vpu?\n");
        return false;
    }
    initializeScheduledCycleTimes(table);
    if (!resetProcessorThroughput(program)) {
        reportError("failed to reset for emu\n");
        return false;
    }
    bool lastOk = true;
    if (!stream->branchType) {
        if (stream->endType != 1)
            lastOk = false;
    } else {
        lastOk = false;
    }
    int curTick = 0;
    int logLen = table->getRawOrSyntheticLogicalLength();
    ol_s *inflight[1024];
    for (int rl = table->getRangeStart(); rl != table->getRangeEnd(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)table)->getIE_ck(rl);
        if (row->skip) {
            reportError("skip already set for line\n");
            return false;
        }
        if (!row->countEntries()) {
            int tNow = row->cycleTime;
            int n = program->proc.gatherInflight(inflight);
            if (n < 0) {
                reportError("error gathering inflight data\n");
                return false;
            }
            int m = table->getRangeEnd();
            int k;
            for (k = 0; k != n; k++) {
                ol_s *operandLocation = inflight[k];
                Instr *instruction = operandLocation->instruction;
                Operand *operand = operandAtLocation(operandLocation);
                RegPoolEntry *reg = operand->assignedRegister;
                int part = operandLocation->componentIndex;
                int lat = operandLocation->cycleOffset;
                if (reg->tracksHotOutputPaths) {
                    int tv = instruction->baseIssueSpacing;
                    if (tv <= 6)
                        exitWithError("bad throught value in register\n");
                    int thr = lat;
                    if (reg->registerIndex == 0x34)
                        thr--;
                    else if (reg->registerIndex != 0x32)
                        exitWithError("non p-q has throughput?\n");
                    int l = rl + 1;
                    wtRow *followingRow = row;
                    while (followingRow++, l < table->getRangeEnd()) {
                        int t = followingRow->cycleTime;
                        if (t > tNow + thr)
                            break;
                        if (followingRow->outputMaskFor(reg, 0xff)) {
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
                for (ol_s *userLocation = operand->parts[part].operandLocations.firstInternalLocation(); userLocation && !userLocation->isExt(); userLocation = userLocation->next) {
                    Instr *userInstruction = userLocation->instruction;
                    if (operandAtLocation(userLocation)->externalBlockModeFlag && operand->outputLatency > 1) {
                        int ul = userInstruction->position.linearRow;
                        int tu = table->ctime(ul);
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
            if (rl == table->getRangeEnd() - 1 && !lastOk)
                ok = false;
            for (k = 0; k != n && ok; k++) {
                ol_s *operandLocation = inflight[k];
                Operand *operand = operandAtLocation(operandLocation);
                int part = operandLocation->componentIndex;
                int lat = operandLocation->cycleOffset;
                ol_s *userLocation = operand->parts[part].operandLocations.firstInternalLocation();
                if (userLocation) {
                    Instr *userInstruction = userLocation->instruction;
                    Operand *userOperand = operandAtLocation(userLocation);
                    int ul = userInstruction->position.linearRow;
                    int tu = table->ctime(ul) - 1;
                    if (!operand->externalBlockModeFlag && !userOperand->externalBlockModeFlag && ul < m && tNow + lat > tu)
                        ok = false;
                }
            }
            if (ok && rl < table->getRangeEnd() - 1 && rl + 1 < m && row[1].getMinimumExtraLatencyAge() == 1)
                ok = false;
            if (m < table->getRangeEnd()) {
                int tm = table->ctime(m);
                wtRow *scannedRow = row + 1;
                for (int l = rl + 1; l < m && ok; l++, scannedRow++) {
                    int base = scannedRow->cycleTime - 1;
                    for (int pp = 0; pp != 2; pp++) {
                        Instr *instruction = scannedRow->getEntry(pp);
                        if (!instruction)
                            continue;
                        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                            Operand *otherOperand = &instruction->operands[operandIndex];
                            RegPoolEntry *writerRegister = registerOperand(otherOperand, 2);
                            if (otherOperand->reductionHazardMask)
                                ok = false;
                            if (writerRegister && !otherOperand->externalBlockModeFlag) {
                                int t = base + otherOperand->outputLatency;
                                if (t < tm) {
                                    int l2 = m;
                                    wtRow *r2 = (wtRow *)((cls_unk30 *)table)->getIE_ck(m);
                                    while (l2 < table->getRangeEnd() && r2->cycleTime < t) {
                                        r2++;
                                        l2++;
                                    }
                                    if (l2 < table->getRangeEnd()) {
                                        if (r2->cycleTime == t) {
                                            if (r2->inputMaskFor(writerRegister, 0xff))
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
                    wtRow *candidateRow = row + 1;
                    int l = rl + 1;
                    do {
                        int base = candidateRow->cycleTime - 1;
                        for (int pp = 0; pp != 2; pp++) {
                            Instr *instruction = GetI((Instr **)candidateRow, pp);
                            if (instruction) {
                                short e = instruction->externalInputConstraint;
                                if (e < instruction->externalInputPairedConstraint)
                                    e = instruction->externalInputPairedConstraint;
                                if (e > base) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                        l++;
                        candidateRow++;
                    } while (l < m && ok);
                }
                if (ok) {
                    if (m >= table->getRangeEnd()) {
                        int last = table->getRawOrSyntheticLogicalLength() - 1;
                        for (int l = table->getRangeStart(); l < m && ok; l++) {
                            wtRow *sampledRow = (wtRow *)((cls_unk30 *)table)->getIE_ck(l);
                            int dist = last - row->cycleTime;
                            if (dist < 0)
                                exitWithError("distance to end appears to be less than zero\n");
                            for (int pp = 0; pp != 2; pp++) {
                                Instr *instruction = GetI((Instr **)sampledRow, pp);
                                if (instruction) {
                                    short e = instruction->externalOutputConstraint;
                                    if (e < instruction->externalOutputPairedConstraint)
                                        e = instruction->externalOutputPairedConstraint;
                                    if (e < dist) {
                                        ok = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (ok) {
                        if (m >= table->getRangeEnd()) {
                            int last = table->getRawOrSyntheticLogicalLength() - 1;
                            if (last < table->externalMinimumLength)
                                ok = false;
                            if (stream->endType != 1 && last <= 1)
                                ok = false;
                        }
                        if (ok) {
                            row->skip = 1;
                            splitCtime(table, rl, m);
                            if (m >= table->getRangeEnd())
                                table->declaredRawLength = --logLen;
                        }
                    }
                }
            }
        }
        int d = emuInst(program, row, rl, false, false);
        if (d < 0) {
            reportError("failed to emu an ientry\n");
            return false;
        }
        curTick += d;
        if (curTick != row->cycleTime + 1) {
            reportError("unexpected time !.. currTick=%d cTime=%d\n", curTick, row->cycleTime);
            return false;
        }
        if (curTick != program->emulatedCycleCount) {
            reportError("currTick doesn't match cpu\n");
            return false;
        }
    }
    if (table->getRawOrSyntheticLogicalLength() != program->emulatedCycleCount) {
        reportError("at end of table logical len not correct\n");
        return false;
    }
    return true;
}

void initializeScheduledCycleTimes(workTable *table)
{
    wtRow *row = table->rows + table->rangeStart;
    for (int l = table->rangeStart; l != table->rangeEnd; l++, row++) {
        int rel = l - table->rangeStart;
        for (int p = 0; p != 2; p++) {
            Instr *instruction = GetI((Instr **)row, p);
            if (instruction && instruction->scheduledCycle != rel)
                exitWithError("l=%d exec time mismatch in instruction %d!=%d\n",
                              l, instruction->scheduledCycle, rel);
        }
        short *time = &row->cycleTime;
        *time = l - table->rangeStart;
    }
}

void splitCtime(workTable *table, int rowIndex, int m)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    if (m <= rowIndex)
        exitWithError("m<l for splitCtime()\n");
    if (m > table->rangeEnd)
        m = table->rangeEnd;
    wtRow *row = &table->rows[rowIndex];
    for (; rowIndex < m; rowIndex++, row++)
        row->cycleTime--;
}

int getCtime(workTable *table, int rowIndex)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    return table->rows[rowIndex].cycleTime;
}

