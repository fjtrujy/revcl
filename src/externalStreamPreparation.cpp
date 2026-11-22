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
bool insertRegisterCapInstruction(int line, int pipe, int mask);
RegPool *getRegisterPoolByIndex(unsigned t);
bool resetProcessorThroughput(Program *p);
int emulateInstructionRow(Program *p, wtRow *row, int line, bool uta, bool commit);
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
    RegPool *hardwarePool = getRegisterPoolByIndex(10);
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
    for (int rowIndex = table->getRangeStart(); rowIndex != table->getRangeEnd(); rowIndex++) {
        wtRow *row = (wtRow *)((cls_unk30 *)table)->getIE_ck(rowIndex);
        if (row->skip) {
            reportError("skip already set for line\n");
            return false;
        }
        if (!row->countEntries()) {
            int tNow = row->cycleTime;
            int inflightCount = program->proc.gatherInflight(inflight);
            if (inflightCount < 0) {
                reportError("error gathering inflight data\n");
                return false;
            }
            int rangeEnd = table->getRangeEnd();
            int inflightIndex;
            for (inflightIndex = 0; inflightIndex != inflightCount; inflightIndex++) {
                ol_s *operandLocation = inflight[inflightIndex];
                Instr *instruction = operandLocation->instruction;
                Operand *operand = operandAtLocation(operandLocation);
                RegPoolEntry *registerEntry = operand->assignedRegister;
                int part = operandLocation->componentIndex;
                int lat = operandLocation->cycleOffset;
                if (registerEntry->tracksHotOutputPaths) {
                    int issueSpacing = instruction->baseIssueSpacing;
                    if (issueSpacing <= 6)
                        exitWithError("bad throught value in register\n");
                    int thr = lat;
                    if (registerEntry->registerIndex == 0x34)
                        thr--;
                    else if (registerEntry->registerIndex != 0x32)
                        exitWithError("non p-q has throughput?\n");
                    int nextRowIndex = rowIndex + 1;
                    wtRow *followingRow = row;
                    while (followingRow++, nextRowIndex < table->getRangeEnd()) {
                        int followingCycleTime = followingRow->cycleTime;
                        if (followingCycleTime > tNow + thr)
                            break;
                        if (followingRow->outputMaskFor(registerEntry, 0xff)) {
                            if (followingCycleTime != tNow + thr) {
                                reportError("unexpected thruput stall detected\n");
                                return false;
                            }
                            if (nextRowIndex < rangeEnd)
                                rangeEnd = nextRowIndex;
                            break;
                        }
                        nextRowIndex++;
                    }
                }
                for (ol_s *userLocation = operand->parts[part].operandLocations.firstInternalLocation(); userLocation && !userLocation->isExt(); userLocation = userLocation->next) {
                    Instr *userInstruction = userLocation->instruction;
                    if (operandAtLocation(userLocation)->externalBlockModeFlag && operand->outputLatency > 1) {
                        int userRow = userInstruction->position.linearRow;
                        int userCycle = table->ctime(userRow);
                        if (userCycle <= tNow) {
                            reportError("user is before now .. tUser=%d tNow = %d\n", userCycle, tNow);
                            return false;
                        }
                        if (tNow + lat > userCycle) {
                            reportError("found an unexpected stall\n");
                            return false;
                        }
                        if (tNow + lat == userCycle && userRow < rangeEnd)
                            rangeEnd = userRow;
                    }
                }
            }
            bool ok = true;
            if (rowIndex == table->getRangeEnd() - 1 && !lastOk)
                ok = false;
            for (inflightIndex = 0; inflightIndex != inflightCount && ok; inflightIndex++) {
                ol_s *operandLocation = inflight[inflightIndex];
                Operand *operand = operandAtLocation(operandLocation);
                int part = operandLocation->componentIndex;
                int lat = operandLocation->cycleOffset;
                ol_s *userLocation = operand->parts[part].operandLocations.firstInternalLocation();
                if (userLocation) {
                    Instr *userInstruction = userLocation->instruction;
                    Operand *userOperand = operandAtLocation(userLocation);
                    int consumerRow = userInstruction->position.linearRow;
                    int consumerCycle = table->ctime(consumerRow) - 1;
                    if (!operand->externalBlockModeFlag && !userOperand->externalBlockModeFlag && consumerRow < rangeEnd && tNow + lat > consumerCycle)
                        ok = false;
                }
            }
            if (ok && rowIndex < table->getRangeEnd() - 1 && rowIndex + 1 < rangeEnd && row[1].getMinimumExtraLatencyAge() == 1)
                ok = false;
            if (rangeEnd < table->getRangeEnd()) {
                int boundaryCycle = table->ctime(rangeEnd);
                wtRow *scannedRow = row + 1;
                for (int scanIndex = rowIndex + 1; scanIndex < rangeEnd && ok; scanIndex++, scannedRow++) {
                    int base = scannedRow->cycleTime - 1;
                    for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                        Instr *instruction = scannedRow->getEntry(pipeIndex);
                        if (!instruction)
                            continue;
                        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                            Operand *otherOperand = &instruction->operands[operandIndex];
                            RegPoolEntry *writerRegister = registerOperand(otherOperand, 2);
                            if (otherOperand->reductionHazardMask)
                                ok = false;
                            if (writerRegister && !otherOperand->externalBlockModeFlag) {
                                int readyCycle = base + otherOperand->outputLatency;
                                if (readyCycle < boundaryCycle) {
                                    int l2 = rangeEnd;
                                    wtRow *r2 = (wtRow *)((cls_unk30 *)table)->getIE_ck(rangeEnd);
                                    while (l2 < table->getRangeEnd() && r2->cycleTime < readyCycle) {
                                        r2++;
                                        l2++;
                                    }
                                    if (l2 < table->getRangeEnd()) {
                                        if (r2->cycleTime == readyCycle) {
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
                if (rowIndex + 1 < rangeEnd) {
                    wtRow *candidateRow = row + 1;
                    int candidateRowIndex = rowIndex + 1;
                    do {
                        int base = candidateRow->cycleTime - 1;
                        for (int candidatePipeIndex = 0; candidatePipeIndex != 2; candidatePipeIndex++) {
                            Instr *instruction = getInstructionInPipe((Instr **)candidateRow, candidatePipeIndex);
                            if (instruction) {
                                short inputConstraint = instruction->externalInputConstraint;
                                if (inputConstraint < instruction->externalInputPairedConstraint)
                                    inputConstraint = instruction->externalInputPairedConstraint;
                                if (inputConstraint > base) {
                                    ok = false;
                                    break;
                                }
                            }
                        }
                        candidateRowIndex++;
                        candidateRow++;
                    } while (candidateRowIndex < rangeEnd && ok);
                }
                if (ok) {
                    if (rangeEnd >= table->getRangeEnd()) {
                        int last = table->getRawOrSyntheticLogicalLength() - 1;
                        for (int sampledRowIndex = table->getRangeStart(); sampledRowIndex < rangeEnd && ok; sampledRowIndex++) {
                            wtRow *sampledRow = (wtRow *)((cls_unk30 *)table)->getIE_ck(sampledRowIndex);
                            int dist = last - row->cycleTime;
                            if (dist < 0)
                                exitWithError("distance to end appears to be less than zero\n");
                            for (int sampledPipeIndex = 0; sampledPipeIndex != 2; sampledPipeIndex++) {
                                Instr *instruction = getInstructionInPipe((Instr **)sampledRow, sampledPipeIndex);
                                if (instruction) {
                                    short outputConstraint = instruction->externalOutputConstraint;
                                    if (outputConstraint < instruction->externalOutputPairedConstraint)
                                        outputConstraint = instruction->externalOutputPairedConstraint;
                                    if (outputConstraint < dist) {
                                        ok = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (ok) {
                        if (rangeEnd >= table->getRangeEnd()) {
                            int last = table->getRawOrSyntheticLogicalLength() - 1;
                            if (last < table->externalMinimumLength)
                                ok = false;
                            if (stream->endType != 1 && last <= 1)
                                ok = false;
                        }
                        if (ok) {
                            row->skip = 1;
                            splitCtime(table, rowIndex, rangeEnd);
                            if (rangeEnd >= table->getRangeEnd())
                                table->declaredRawLength = --logLen;
                        }
                    }
                }
            }
        }
        int emulationResult = emulateInstructionRow(program, row, rowIndex, false, false);
        if (emulationResult < 0) {
            reportError("failed to emu an ientry\n");
            return false;
        }
        curTick += emulationResult;
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
    for (int rowIndex = table->rangeStart; rowIndex != table->rangeEnd; rowIndex++, row++) {
        int rel = rowIndex - table->rangeStart;
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            Instr *instruction = getInstructionInPipe((Instr **)row, pipeIndex);
            if (instruction && instruction->scheduledCycle != rel)
                exitWithError("l=%d exec time mismatch in instruction %d!=%d\n",
                              rowIndex, instruction->scheduledCycle, rel);
        }
        short *time = &row->cycleTime;
        *time = rowIndex - table->rangeStart;
    }
}

void splitCtime(workTable *table, int rowIndex, int cycleTime)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    if (cycleTime <= rowIndex)
        exitWithError("m<l for splitCtime()\n");
    if (cycleTime > table->rangeEnd)
        cycleTime = table->rangeEnd;
    wtRow *row = &table->rows[rowIndex];
    for (; rowIndex < cycleTime; rowIndex++, row++)
        row->cycleTime--;
}

int getCtime(workTable *table, int rowIndex)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    return table->rows[rowIndex].cycleTime;
}

