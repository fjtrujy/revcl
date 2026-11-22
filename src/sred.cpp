#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int countSetBits(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);

static inline bool isRawTable(workTable *scheduleTable)
{
    return scheduleTable->codeModeValue == 3;
}

static inline bool isPipeMode(workTable *scheduleTable)
{
    return isRawTable(scheduleTable) && (scheduleTable->rawSchedulingMode == 1 || scheduleTable->rawSchedulingMode == 3);
}

inline bool atEarliest(workTable *scheduleTable, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int e = earliestInsert(scheduleTable, instruction, &found);
    return found && e == instruction->position.loopRow;
}

bool isAtEarliest(workTable *scheduleTable, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int e = earliestInsert(scheduleTable, instruction, &found);
    return found && e == instruction->position.loopRow;
}

static inline void dumpSredInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

extern const char sredEarliestAfterError[];
extern const char sredMoveInstError[];
extern const char sredLatestBeforeError[];
extern const char sredBadlyPlacedError[];

bool sredInstr(workTable *scheduleTable, Instr *instruction, bool up)
{
    InstructionPosition pos;
    int iline = instruction->position.loopRow;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (up) {
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
        wtRow *row = scheduleTable->getRowByIndex(tline);
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

bool sredTable(workTable *scheduleTable)
{
    if (!isPipeMode(scheduleTable))
        return true;
    int i = scheduleTable->rangeStart;
    int n = scheduleTable->rangeEnd;
    if (i != n) {
        i = n = scheduleTable->loopSize;
        if (i) {
            reportError("can't do modular reduction\n");
            return false;
        }
        setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, 0);

        int changed;
        do {
            changed = false;
            for (i = scheduleTable->rangeStart; i != scheduleTable->rangeEnd; i++) {
                Instr *instruction = scheduleTable->getRowByIndex(i)->onlyEntry();
                if (instruction) {
                    if (instruction->position.loopRow != i) {
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
            for (i = scheduleTable->rangeEnd - 1; i >= scheduleTable->rangeStart; i--) {
                Instr *in = scheduleTable->getRowByIndex(i)->onlyEntry();
                if (in && !atEarliest(scheduleTable, in)) {
                    if (in->position.loopRow != i) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(scheduleTable, in, false)) {
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
iInfo *selectedCapOpcodeInfo;
workTable *capInsertionWorkTable;
Instr *selectedCapOriginalInstruction;
RegPoolEntry *selectedCapRegister;
int selectedCapMask;
