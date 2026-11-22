#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRangeFrom(workTable *t);
void resetRange();

static inline void dumpRawInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

bool workTable::prepRawTable()
{
    int mode = codeModeValue;
    if ((unsigned char)mode != 3) {
        const char *modeName = codeModeName(mode);
        reportError("prepRawTable called - but mode is not raw %s\n", modeName);
    }
    if (!rows) {
        reportError("iTable::prepRawTable - table not ready\n");
        return 0;
    }
    if (!setRangeFrom(this)) {
        reportError("set raw table - failed to set iLoc range\n");
        return 0;
    }
    linearInstructions.reset();
    completedInstructions.reset();
    pendingInstructions.reset();
    moveListHead = 0;
    pendingMoveCount = 0;
    mainLoopId = 0;
    resetCounts();
    if (rangeStart) {
        reportError("initial start - table not based at zero\n");
        return 0;
    }
    int maxLid = 0;
    int tlen = rangeEnd;
    if (tlen < 0) {
        reportError("prepTable - tlen<0\n");
        return 0;
    }
    if (tlen == 0)
        rawSchedulingMode = 1;
    for (int rowIndex = 0; rowIndex < rangeEnd; rowIndex++) {
        wtRow *workRow = (wtRow *)((cls_unk30 *)this)->getIE_ck(rowIndex);
        if (!workRow) {
            reportError("iTable::prepTable - failed to get ientry\n");
            return 0;
        }
        if (workRow->skip) {
            reportError("bSkip set for preptabl\n");
            return 0;
        }
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *rowInstruction = workRow->entries[entryIndex].instruction;
            if (!rowInstruction)
                continue;
            if (rowInstruction->syntheticKind) {
                reportError("synth instruction in preptab\n");
                rowInstruction->dumpInstructionContext();
                abortAfterError();
            }
            if (rowInstruction->parsedLoopId != -1) {
                reportError("parseLid isn't cleared in prepTab\n");
                rowInstruction->dumpOperandDiagnostics(false);
                exitAfterCleanup(-1);
            }
            int lid = 0;
            int instructionRow;
            InstructionPosition *loc;
            if (loopSize) {
                if (!rowInstruction->isNop())
                    lid = rowInstruction->position.loopIteration;
                if (lid < 0) {
                    reportError("lid was <0 during table setup\n");
                    rowInstruction->dumpInstructionContext();
                    return 0;
                }
                loc = &rowInstruction->position;
                loc->loopIteration = lid;
                instructionRow = lid * loopSize + rowIndex;
            } else {
                loc = &rowInstruction->position;
                lid = loc->loopIteration;
                if (lid) {
                    reportError("non zero lid for non-loop %d\n", lid);
                    dumpRawInstruction(rowInstruction);
                    return 0;
                }
                loc->loopIteration = 0;
                instructionRow = rowIndex;
            }
            if (lid > maxLid)
                maxLid = lid;
            InstructionPosition pos;
            pos.setFromLinearRow(instructionRow);
            pos.setInstructionPipe(entryIndex);
            rowInstruction->position.linearRow = pos.linearRow;
            signed char loopIteration = pos.loopIteration;
            loc->loopRow = pos.loopRow;
            loc->loopIteration = loopIteration;
            loc->pipelineIndex = pos.pipelineIndex;
            if (loopIteration != lid) {
                reportError("iTable::initialSetup - lid was bad (%d!=%d) lsz=%d rl=%d il=%d\n",
                              loopIteration, lid, loopSize, instructionRow, rowIndex);
                return 0;
            }
            if (pos.loopRow != rowIndex) {
                reportError("iTable::initialSetup - iline was bad\n");
                return 0;
            }
            if (rowInstruction->ownerTable) {
                if (rowInstruction->ownerTable != (cls_unk30 *)this) {
                    reportError("iTable::prep - didn't own this instruction\n");
                    return 0;
                }
            } else if (!rowInstruction->moveRecordWord) {
                reportError("iTable::prep - instruction not a move, but owner wasn't set\n");
                return 0;
            }
            rowInstruction->ownerTable = 0;
            rowInstruction->setOwner((cls_unk30 *)this);
            if (!logAddLinear_Tail(rowInstruction)) {
                extern const char prepRawAddInstrError[];
                reportError(prepRawAddInstrError);
                rowInstruction->dumpInstructionContext();
                return 0;
            }
        }
    }
    if (loopSize)
        mainLoopId = maxLid;
    else if (maxLid) {
        reportError("iTable::initialSetup - maxLid set - but not loopsize!\n");
        return 0;
    }
    resetRange();
    return 1;
}
