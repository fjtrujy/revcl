#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(ScheduleTable *wt, Instr *in, bool *found);
int latestInsert(ScheduleTable *wt, Instr *in, bool *found);
int checkPlacementInAllowedPipelines(ScheduleTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int countSetBits(unsigned int x);
RegisterEntry *getPoolZeroRegister(unsigned int type);

static inline bool isRawTable(ScheduleTable *scheduleTable)
{
    return scheduleTable->codeModeValue == VCL_CODE_RAW;
}

static inline bool isPipeMode(ScheduleTable *scheduleTable)
{
    return isRawTable(scheduleTable) && (scheduleTable->rawSchedulingMode == VCL_RAW_COMPILER_FREE || scheduleTable->rawSchedulingMode == VCL_RAW_USER_FREE);
}

inline bool atEarliest(ScheduleTable *scheduleTable, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int earliestAllowedRow = earliestInsert(scheduleTable, instruction, &found);
    return found && earliestAllowedRow == instruction->position.loopRow;
}

bool isAtEarliest(ScheduleTable *scheduleTable, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int earliestAllowedRow = earliestInsert(scheduleTable, instruction, &found);
    return found && earliestAllowedRow == instruction->position.loopRow;
}

static inline void dumpSredInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

extern const char sredEarliestAfterError[];
extern const char sredMoveInstError[];
extern const char sredLatestBeforeError[];
extern const char sredBadlyPlacedError[];

bool sredInstr(ScheduleTable *scheduleTable, Instr *instruction, bool reduceUpward)
{
    InstructionPosition pos;
    int iline = instruction->position.loopRow;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (reduceUpward) {
        tline = earliestInsert(scheduleTable, instruction, &found);
        if (tline > iline) {
            short mfs = instruction->externalInputConstraint < instruction->externalInputPairedConstraint ? instruction->externalInputPairedConstraint : instruction->externalInputConstraint;
            reportError(sredEarliestAfterError,
                          tline, iline, mfs);
            instruction->dumpOperandDiagnostics(false);
            reportError("iline=%d tline=%d\n", iline, tline);
            return false;
        }
        step = 1;
        upwards = true;
    } else {
        tline = latestInsert(scheduleTable, instruction, &found);
        if (tline < iline) {
            reportError(sredLatestBeforeError, tline, iline);
            dumpSredInstruction(instruction);
            return false;
        }
        step = -1;
    }

    if (tline == iline)
        return true;
    if (found) {
        pos.setFromLinearRow(tline);
        if (!checkPlacementInAllowedPipelines(scheduleTable, instruction, &pos, upwards))
            goto move;
        tline += step;
    }
    for (; tline != iline; tline += step) {
        pos.setFromLinearRow(tline);
        ScheduleRow *row = scheduleTable->getRowByIndex(tline);
        if (row->countEntries() == 1
            && !checkPlacementInAllowedPipelines(scheduleTable, instruction, &pos, upwards))
            goto move;
    }
    return true;

move:
    if (!scheduleTable->takeOut(instruction) || !moveLocks(instruction, &pos) || !scheduleTable->putIn(instruction)) {
        reportError(sredMoveInstError);
        return false;
    }
    return true;
}

bool sredTable(ScheduleTable *scheduleTable)
{
    if (!isPipeMode(scheduleTable))
        return true;
    int rowIndex = scheduleTable->rangeStart;
    int rangeEnd = scheduleTable->rangeEnd;
    if (rowIndex != rangeEnd) {
        rowIndex = rangeEnd = scheduleTable->loopSize;
        if (rowIndex) {
            reportError("can't do modular reduction\n");
            return false;
        }
        setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, 0);

        int changed;
        do {
            changed = false;
            for (rowIndex = scheduleTable->rangeStart; rowIndex != scheduleTable->rangeEnd; rowIndex++) {
                Instr *instruction = scheduleTable->getRowByIndex(rowIndex)->onlyEntry();
                if (instruction) {
                    if (instruction->position.loopRow != rowIndex) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(scheduleTable, instruction, true)) {
                        reportError("failed to sred a line\n");
                        return false;
                    }
                }
            }
        } while (changed);

        do {
            changed = false;
            for (rowIndex = scheduleTable->rangeEnd - 1; rowIndex >= scheduleTable->rangeStart; rowIndex--) {
                Instr *reductionInstruction = scheduleTable->getRowByIndex(rowIndex)->onlyEntry();
                if (reductionInstruction && !atEarliest(scheduleTable, reductionInstruction)) {
                    if (reductionInstruction->position.loopRow != rowIndex) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(scheduleTable, reductionInstruction, false)) {
                        reportError("failed to sred a line\n");
                        return false;
                    }
                }
            }
        } while (changed);
    }
    return true;
}

int selectedCapPipelineProperties;
OpcodeDescriptor *selectedCapOpcodeInfo;
ScheduleTable *capInsertionWorkTable;
Instr *selectedCapOriginalInstruction;
RegisterEntry *selectedCapRegister;
int selectedCapMask;
