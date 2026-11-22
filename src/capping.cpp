#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"

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

int insertAllRegisterCaps(Program *program)
{
    int total = 0;
    InstructionStream *currentStream;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        currentStream->externalSchedulingPrepared = 0;
        if (!currentStream->usesFreeRawLayout())
            currentStream->externalSchedulingPrepared = 1;
    }
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (currentStream->externalSchedulingPrepared)
            continue;
        if (!prepExtInfo(program, currentStream, 0)) {
            reportError("failed to prep external info for stream %s\n", currentStream->streamName());
            return -1;
        }
        int insertedCapCount = insertStreamRegisterCaps(program, currentStream);
        if (insertedCapCount < 0) {
            reportError("failed to sred a table from %s\n", currentStream->streamName());
            return -1;
        }
        currentStream->externalSchedulingPrepared = 1;
        total += insertedCapCount;
    }
    return total;
}

int insertStreamRegisterCaps(Program *program, InstructionStream *stream)
{
    int added = 0;
    if (!stream->workTablePtr) {
        reportError("work table not set in %s\n", stream->streamName());
        exitAfterCleanup(-1);
    }
    ScheduleTable *scheduleTable = stream->workTablePtr;
    if (!scheduleTable) {
        reportError("no work table \n");
        return -1;
    }
    int len = scheduleTable->getRangeEnd() - scheduleTable->getRangeStart();
    if (len == 0)
        return 0;
    if (scheduleTable->externalMinimumLength < 0) {
        reportError("the minimal length is -ve for ext\n");
        return -1;
    }
    if (len < scheduleTable->externalMinimumLength) {
        reportError("table is lready too short (%d<%d)\n", len, scheduleTable->externalMinimumLength);
        return -1;
    }
    initializeScheduledCycleTimes(scheduleTable);
    for (int rowIndex = scheduleTable->getRangeStart(); rowIndex != scheduleTable->getRangeEnd(); rowIndex++) {
        ScheduleRow *row = (ScheduleRow *)((InstructionOwnerTableView *)scheduleTable)->getIE_ck(rowIndex);
        int cycleTime = row->ctime();
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            Instr *rowInstruction = getInstructionInPipe((Instr **)row, pipeIndex);
            if (!rowInstruction)
                continue;
            Operand *operand;
            for (int operandIndex = 0; operandIndex != rowInstruction->operandCount; operandIndex++) {
                operand = &rowInstruction->operands[operandIndex];
                RegisterEntry *registerEntry = registerOperand(operand, 2);
                OperandLocation *live = operand->parts[0].operandLocations.head;
                if (!registerEntry || operand->externalBlockModeFlag || !live)
                    continue;
                RegisterEntry *capReg = 0;
                int capMask = 0;
                int bits = findCap(scheduleTable, rowInstruction, operandIndex, &capReg, &capMask);
                if (bits < 0) {
                    reportError("cancap failed\n");
                    return -1;
                }
                if (!bits)
                    continue;
                int need = operand->outputLatency;
                if (registerEntry->registerIndex == 0x34 && need > 1)
                    need--;
                int line = rowIndex + 1;
                ScheduleRow *workRow = row + 1;
                int limit = need + cycleTime;
                int bestMask = 0, bestLine = -1, bestPipe = -1, bestCount = 0, count = 0;
                while (line < scheduleTable->getRangeEnd() && capMask) {
                    if (workRow->cycleTime >= limit) {
                        for (int availablePipeIndex = 0; availablePipeIndex != 2; availablePipeIndex++) {
                            if (((bits >> availablePipeIndex) & 1) && !getInstructionInPipe((Instr **)workRow, availablePipeIndex)) {
                                if ((!workRow->countEntries() && bestLine < 0) || workRow->countEntries()) {
                                    bestLine = line;
                                    bestCount = count;
                                    bestPipe = availablePipeIndex;
                                    bestMask = capMask;
                                }
                                break;
                            }
                        }
                    }
                    if (workRow->inputMaskFor(registerEntry, 0xff))
                        break;
                    if (workRow->countEntries())
                        count++;
                    capMask &= ~workRow->outputMask(capReg);
                    line++;
                    workRow++;
                }
                if (bestLine < 0)
                    continue;
                if (bestCount < need) {
                    if (!insertRegisterCapInstruction(bestLine, bestPipe, bestMask)) {
                        reportError("failed to add cap instruction\n");
                        return -1;
                    }
                    added++;
                }
            }
        }
    }
    if (added && !scheduleTable->relinkRawIt()) {
        reportError("failed to relink table after adding cap instruction\n");
        return 0;
    }
    return added;
}
