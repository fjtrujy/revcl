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

static inline bool isRawTable(workTable *wt)
{
    return wt->codeModeValue == 3;
}

static inline bool isPipeMode(workTable *wt)
{
    return isRawTable(wt) && (wt->rawSchedulingMode == 1 || wt->rawSchedulingMode == 3);
}

inline bool atEarliest(workTable *wt, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int e = earliestInsert(wt, instruction, &found);
    return found && e == instruction->position.loopRow;
}

bool isAtEarliest(workTable *wt, Instr *instruction)
{
    bool found = false;
    if (!instruction)
        return false;
    int e = earliestInsert(wt, instruction, &found);
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

bool sredInstr(workTable *wt, Instr *instruction, bool up)
{
    InstructionPosition pos;
    int iline = instruction->position.loopRow;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (up) {
        tline = earliestInsert(wt, instruction, &found);
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
        tline = latestInsert(wt, instruction, &found);
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
        if (!checkPlacementInAllowedPipelines(wt, instruction, &pos, upwards))
            goto move;
        tline += step;
    }
    for (; tline != iline; tline += step) {
        pos.setFromLinearRow(tline);
        wtRow *row = wt->getRowByIndex(tline);
        if (row->countEntries() == 1
            && !checkPlacementInAllowedPipelines(wt, instruction, &pos, upwards))
            goto move;
    }
    return true;

move:
    if (!wt->takeOut(instruction) || !moveLocks(instruction, &pos) || !wt->putIn(instruction)) {
        reportError(sredMoveInstError);
        return false;
    }
    return true;
}

bool sredTable(workTable *wt)
{
    if (!isPipeMode(wt))
        return true;
    int i = wt->rangeStart;
    int n = wt->rangeEnd;
    if (i != n) {
        i = n = wt->loopSize;
        if (i) {
            reportError("can't do modular reduction\n");
            return false;
        }
        setRange(wt->rangeStart, wt->rangeEnd, 0);

        int changed;
        do {
            changed = false;
            for (i = wt->rangeStart; i != wt->rangeEnd; i++) {
                Instr *instruction = wt->getRowByIndex(i)->onlyEntry();
                if (instruction) {
                    if (instruction->position.loopRow != i) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(wt, instruction, true)) {
                        reportError("failed to sred a line\n");
                        return false;
                    }
                }
            }
        } while (changed);

        do {
            changed = false;
            for (i = wt->rangeEnd - 1; i >= wt->rangeStart; i--) {
                Instr *in = wt->getRowByIndex(i)->onlyEntry();
                if (in && !atEarliest(wt, in)) {
                    if (in->position.loopRow != i) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(wt, in, false)) {
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
