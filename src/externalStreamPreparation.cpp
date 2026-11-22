#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"
#include "ol_s_inline.h"

bool prepExtInfo(Program *p, InstructionStream *s, bool b);
int insertStreamRegisterCaps(Program *p, InstructionStream *s);
void initializeScheduledCycleTimes(ScheduleTable *wt);
bool removeNops(Program *p, InstructionStream *s);
int findCap(ScheduleTable *wt, Instr *in, int opIdx, RegisterEntry **capReg, int *capMask);
bool insertRegisterCapInstruction(int line, int pipe, int mask);
RegPool *getRegisterPoolByIndex(unsigned t);
bool resetProcessorThroughput(Program *p);
int emulateInstructionRow(Program *p, ScheduleRow *row, int line, bool uta, bool commit);
void splitCtime(ScheduleTable *wt, int l, int m);


bool prepareExternalSchedulingForProgram(Program *program)
{
    InstructionStream *stream;
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

bool removeNops(Program *program, InstructionStream *stream)
{
    if (!stream->workTablePtr) {
        reportError("work table not set in %s\n", stream->streamName());
        exitAfterCleanup(-1);
    }
    ScheduleTable *table = stream->workTablePtr;
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
        if (stream->endType != VCL_STREAM_END_NULL)
            lastOk = false;
    } else {
        lastOk = false;
    }
    int curTick = 0;
    int logLen = table->getRawOrSyntheticLogicalLength();
    OperandLocation *inflight[1024];
    for (int rowIndex = table->getRangeStart(); rowIndex != table->getRangeEnd(); rowIndex++) {
        ScheduleRow *row = (ScheduleRow *)((InstructionOwnerTableView *)table)->getIE_ck(rowIndex);
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
                OperandLocation *operandLocation = inflight[inflightIndex];
                Instr *instruction = operandLocation->instruction;
                Operand *operand = operandAtLocation(operandLocation);
                RegisterEntry *registerEntry = operand->assignedRegister;
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
                    ScheduleRow *followingRow = row;
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
                for (OperandLocation *userLocation = operand->parts[part].operandLocations.firstInternalLocation(); userLocation && !userLocation->isExt(); userLocation = userLocation->next) {
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
            bool canRemoveInstruction = true;
            if (rowIndex == table->getRangeEnd() - 1 && !lastOk)
                canRemoveInstruction = false;
            for (inflightIndex = 0; inflightIndex != inflightCount && canRemoveInstruction; inflightIndex++) {
                OperandLocation *operandLocation = inflight[inflightIndex];
                Operand *operand = operandAtLocation(operandLocation);
                int part = operandLocation->componentIndex;
                int lat = operandLocation->cycleOffset;
                OperandLocation *userLocation = operand->parts[part].operandLocations.firstInternalLocation();
                if (userLocation) {
                    Instr *userInstruction = userLocation->instruction;
                    Operand *userOperand = operandAtLocation(userLocation);
                    int consumerRow = userInstruction->position.linearRow;
                    int consumerCycle = table->ctime(consumerRow) - 1;
                    if (!operand->externalBlockModeFlag && !userOperand->externalBlockModeFlag && consumerRow < rangeEnd && tNow + lat > consumerCycle)
                        canRemoveInstruction = false;
                }
            }
            if (canRemoveInstruction && rowIndex < table->getRangeEnd() - 1 && rowIndex + 1 < rangeEnd && row[1].getMinimumExtraLatencyAge() == 1)
                canRemoveInstruction = false;
            if (rangeEnd < table->getRangeEnd()) {
                int boundaryCycle = table->ctime(rangeEnd);
                ScheduleRow *scannedRow = row + 1;
                for (int scanIndex = rowIndex + 1; scanIndex < rangeEnd && canRemoveInstruction; scanIndex++, scannedRow++) {
                    int base = scannedRow->cycleTime - 1;
                    for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                        Instr *instruction = scannedRow->getEntry(pipeIndex);
                        if (!instruction)
                            continue;
                        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                            Operand *otherOperand = &instruction->operands[operandIndex];
                            RegisterEntry *writerRegister = registerOperand(otherOperand, 2);
                            if (otherOperand->reductionHazardMask)
                                canRemoveInstruction = false;
                            if (writerRegister && !otherOperand->externalBlockModeFlag) {
                                int readyCycle = base + otherOperand->outputLatency;
                                if (readyCycle < boundaryCycle) {
                                    int l2 = rangeEnd;
                                    ScheduleRow *r2 = (ScheduleRow *)((InstructionOwnerTableView *)table)->getIE_ck(rangeEnd);
                                    while (l2 < table->getRangeEnd() && r2->cycleTime < readyCycle) {
                                        r2++;
                                        l2++;
                                    }
                                    if (l2 < table->getRangeEnd()) {
                                        if (r2->cycleTime == readyCycle) {
                                            if (r2->inputMaskFor(writerRegister, 0xff))
                                                canRemoveInstruction = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (canRemoveInstruction) {
                if (rowIndex + 1 < rangeEnd) {
                    ScheduleRow *candidateRow = row + 1;
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
                                    canRemoveInstruction = false;
                                    break;
                                }
                            }
                        }
                        candidateRowIndex++;
                        candidateRow++;
                    } while (candidateRowIndex < rangeEnd && canRemoveInstruction);
                }
                if (canRemoveInstruction) {
                    if (rangeEnd >= table->getRangeEnd()) {
                        int last = table->getRawOrSyntheticLogicalLength() - 1;
                        for (int sampledRowIndex = table->getRangeStart(); sampledRowIndex < rangeEnd && canRemoveInstruction; sampledRowIndex++) {
                            ScheduleRow *sampledRow = (ScheduleRow *)((InstructionOwnerTableView *)table)->getIE_ck(sampledRowIndex);
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
                                        canRemoveInstruction = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (canRemoveInstruction) {
                        if (rangeEnd >= table->getRangeEnd()) {
                            int last = table->getRawOrSyntheticLogicalLength() - 1;
                            if (last < table->externalMinimumLength)
                                canRemoveInstruction = false;
                            if (stream->endType != VCL_STREAM_END_NULL && last <= 1)
                                canRemoveInstruction = false;
                        }
                        if (canRemoveInstruction) {
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

void initializeScheduledCycleTimes(ScheduleTable *table)
{
    ScheduleRow *row = table->rows + table->rangeStart;
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

void splitCtime(ScheduleTable *table, int rowIndex, int cycleTime)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    if (cycleTime <= rowIndex)
        exitWithError("m<l for splitCtime()\n");
    if (cycleTime > table->rangeEnd)
        cycleTime = table->rangeEnd;
    ScheduleRow *row = &table->rows[rowIndex];
    for (; rowIndex < cycleTime; rowIndex++, row++)
        row->cycleTime--;
}

int getCtime(ScheduleTable *table, int rowIndex)
{
    if (rowIndex < table->rangeStart || rowIndex >= table->rangeEnd)
        exitWithError("nop line is bad\n");
    return table->rows[rowIndex].cycleTime;
}

