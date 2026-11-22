#define VCL_POSTOPT_FLOAT_LENGTH
#include "workTable.h"
#undef VCL_POSTOPT_FLOAT_LENGTH
#include "wtQueue_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);


static inline void dumpOptInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

union RawLength { int value; float bits; };

static inline int rawLengthBits(float value)
{
    RawLength result;
    result.bits = value;
    return result.value;
}

static inline void assignLength(float &dst, int value)
{
    for (;;) {
        RawLength result;
        result.value = value;
        dst = result.bits;
        break;
    }
}

bool workTable::doPostOpt(int rawMode)
{
    if (!rows) {
        reportWarning("doPostOpt() - no attached table!\n");
        return true;
    }
    {
        if (rangeStart < 0 || rangeEnd < 0 || rangeEnd < rangeStart) {
            reportError("bad table start/end\n");
            return false;
        }
        if (codeModeValue != 3) {
            reportError("not mid-code mode for doPostOpt\n");
            return false;
        }
        postOptimizationPrepared = 1;
        if (pendingInstructions.count) {
            reportError("doPostOpt - but there's things still to do\n");
            return false;
        }
        instStream *owner = ownerStream;
        if (!owner) {
            reportError("doPostOpt - not get owner\n");
            return false;
        }
        workTable *scheduleTable = owner->getTable();
        if (!scheduleTable) {
            reportError("failed to get work table from owner\n");
            return false;
        }
        if (scheduleTable == this) {
            reportError("doPostOpt ..parent work table is us!\n");
            return false;
        }
        int expected;
        wtInstrList *lin = &linearInstructions;
        expected = lin->count;
        int count = 0;
        if (owner->getEndType() == 2) {
            if (rangeEnd - rangeStart <= 1)
                rangeEnd = rangeStart + 2;
        } else if (owner->getEndType() == 3) {
            if (rangeEnd - rangeStart <= 1)
                rangeEnd = rangeStart + 2;
        }
        ::setRange(rangeStart, rangeEnd, loopSize);
        int maxLid = 0;
        float length;
        switch (rawMode) {
        default:
            reportError("bad raw mode into doPostOpt()\n");
            return false;
        case 1:
        case 2: {
            int rangeStart = *(volatile int *)&this->rangeStart;
            int rangeEnd = this->rangeEnd;
            *(int *)&length = rangeEnd - rangeStart;
            break;
        }
        case 3:
        case 4:
            assignLength(length, rawLengthBits(scheduleTable->getRawOrSyntheticLogicalLength()));
            break;
        }
        if (!setTableRawMode(rawMode)) {
            reportError("doPostOpt : failed to set raw mode..\n");
            return false;
        }
        int rowIndex;
        for (rowIndex = rangeStart; rowIndex < rangeEnd; rowIndex++) {
            Instr **instructionSlot = ((cls_unk30 *)this)->getIE_ck(rowIndex);
            if (!instructionSlot) {
                reportError("iTable::doPostOpt - failed to get ientry\n");
                return false;
            }
            if (((wtRow *)instructionSlot)->skip) {
                reportError("bSkip is set\n");
                return false;
            }
            for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                Instr *rowInstruction = instructionSlot[pipeIndex];
                if (rowInstruction) {
                    int nBranch;
                    switch (rawMode) {
                    case 2: {
                        int scheduledRow = rowInstruction->position.loopRow;
                        rowInstruction->scheduledCycle = scheduledRow;
                        if (rangeStart) {
                            reportError("postopt - istart set for fixed mode \n");
                            return false;
                        }
                        break;
                    }
                    case 1: {
                        nBranch = -1;
                        int rawScheduledRow = rowInstruction->position.loopRow;
                        rowInstruction->scheduledCycle = rawScheduledRow - rangeStart;
                        break;
                    }
                    case 3:
                    case 4: {
                        Instr *cloneInstruction = rowInstruction->cloneImage;
                        if (!cloneInstruction) {
                            reportError("no work clone set\n");
                            return false;
                        }
                        short scheduledCycle = cloneInstruction->scheduledCycle;
                        if (scheduledCycle < 0 || scheduledCycle >= *(int *)&length) {
                            reportError("emutick bad for raw postopt\n");
                            return false;
                        }
                        rowInstruction->scheduledCycle = scheduledCycle;
                        break;
                    }
                    default:
                        reportError("raw post mode for postopt\n");
                        return false;
                    }
                    if (!rowInstruction->moveRecordWord) {
                        if (rowInstruction->ownerTable != (cls_unk30 *)this) {
                            reportError("owner not this for non-move...%p\n", rowInstruction->ownerTable);
                            dumpOptInstruction(rowInstruction);
                            return false;
                        }
                        if (!logUnlinkFromLinear(rowInstruction)) {
                            reportError("doPostOpt:failed to unlink from linear list\n");
                            return false;
                        }
                        count++;
                    } else {
                        if (rowInstruction->ownerTable) {
                            reportError("owner not 0 for move\n");
                            return false;
                        }
                    }
                    if (!unlinkFromCompletedInstructions((Block *)this, rowInstruction)) {
                        reportError("doPostOpt -  unlink from done list failed\n");
                        return false;
                    }
                    if (rowInstruction->branchFlags) {
                        if (nBranch + 1) {
                            reportError("doPostOpt - more than one branch\n");
                            return false;
                        }
                        if (rowIndex != rangeEnd - 2) {
                            reportError("badly placed branch detected in postopt\n");
                            dump();
                            return false;
                        }
                    }
                    InstructionPosition *loc = &rowInstruction->position;
                    int lid = loc->loopIteration;
                    int row = loc->loopRow;
                    int iloc = rowInstruction->position.linearRow;
                    if (!loopSize && lid) {
                        reportError("i_lid set not in loop\n");
                        return false;
                    }
                    int rebuilt = lid * activeLoopSize;
                    rebuilt += row;
                    if (iloc != rebuilt) {
                        reportError("badly formed iloc in instruction\n");
                        return false;
                    }
                    if (row != rowIndex) {
                        reportError("instruction not where it thinks it is\n");
                        return false;
                    }
                    if (lid > maxLid)
                        maxLid = lid;
                    if (!instructionSlot[pipeIndex]) {
                        reportError("damn - lost instruction at %d:%d!\n", rowIndex, pipeIndex);
                        return false;
                    }
                }
            }
        }
        if (count != expected) {
            reportError("doPostOpt - linear list inconsistency %d!=%d.. \n", count, expected);
            dump();
            return false;
        }
        if (completedInstructions.count) {
            reportError("doPostOpt() - things remained on done list after scanning table\n");
            return false;
        }
        if (lin->count) {
            reportError("doPostOpt() - things remained on Linear list after scanning table\n");
            return false;
        }
        if (loopSize)
            mainLoopId = maxLid;
        else if (maxLid) {
            reportError("iTable::doPostOpt - maxLid set - but not loopsize!\n");
            return false;
        }
        resetCounts();
        for (rowIndex = rangeStart; rowIndex < rangeEnd; rowIndex++) {
            Instr **rowInstructionSlots = ((cls_unk30 *)this)->getIE_ck(rowIndex);
            if (!rowInstructionSlots)
                reportError("can't get ie during postopt\n");
            for (int emissionPipeIndex = 0; emissionPipeIndex != 2; emissionPipeIndex++) {
                Instr *linearInstruction = rowInstructionSlots[emissionPipeIndex];
                if (linearInstruction) {
                    if (!logAddLinear_Tail(linearInstruction)) {
                        reportError("failed to add in to linear tale\n");
                        return false;
                    }
                }
            }
        }
        if (loopSize && rangeEnd - rangeStart != loopSize) {
            reportError("bad loop size for postOpt\n");
            return false;
        }
        declaredRawLength = *(int *)&length;
    }
    return true;
}

static inline bool isRawTable(workTable *scheduleTable)
{
    return scheduleTable->codeModeValue == 3;
}

bool instStream::setOptTable(workTable *scheduleTable)
{
    if (!scheduleTable) {
        reportError("can't set opt table to null ptr - use clear\n");
        abortAfterError();
    }
    if (!isRawTable(scheduleTable)) {
        reportError("setOptTable - table not vsm\n");
        return false;
    }
    workTable *old = getTable();
    if (!old) {
        reportError("no old work table\n");
        return false;
    }
    if (old == scheduleTable) {
        reportError("old and new are same\n");
        return false;
    }
    if (optimizedTable) {
        reportError("stream already has opttable set\n");
        return false;
    }
    for (Instr *originalInstruction = scheduleTable->linearInstructions.head; originalInstruction; originalInstruction = originalInstruction->linearLink.nextInstruction) {
        if (originalInstruction->getOwningStream() != this) {
            reportError("didn't own instruction\n");
            return false;
        }
        if (originalInstruction->optimizedInstructionImageWord) {
            reportError("doPostLinearOpt - optClone set in image?\n");
            return false;
        }
        Instr *cloneInstruction = originalInstruction->cloneImage;
        if (!cloneInstruction) {
            if (!originalInstruction->moveRecordWord) {
                reportError("doPostLinearOpt - inst wasn't cloned, but not a move either\n");
                dumpOptInstruction(originalInstruction);
                return false;
            }
        } else {
            if (cloneInstruction->optimizedInstructionImageWord) {
                reportError("doPostLinear() - original had m_optClone set\n");
                dumpOptInstruction(cloneInstruction);
                return false;
            }
            if (cloneInstruction->getOwningStream() != this) {
                reportError("setOptTable -  cloned image wsn't owned by us\n");
                return false;
            }
            workTable *owner = (workTable *)cloneInstruction->ownerTable;
            if (owner != getTable()) {
                reportError("clone owner wasn't our work table\n");
                return false;
            }
            if (originalInstruction->operandCount != cloneInstruction->operandCount) {
                reportError("operand count mismatch between original and opt image\n");
                return false;
            }
            for (int operandIndex = 0; operandIndex != originalInstruction->operandCount; operandIndex++) {
                char inputDirection = originalInstruction->operands[operandIndex].direction;
                char cio = cloneInstruction->operands[operandIndex].direction;
                if (inputDirection != cio) {
                    if (cio != 1 && inputDirection != 3) {
                        reportError("io type mismatch for something that wasn't a whacked input\n");
                        return false;
                    }
                    originalInstruction->operands[operandIndex].assignedRegister = cloneInstruction->operands[operandIndex].assignedRegister;
                    originalInstruction->operands[operandIndex].direction = cio;
                }
            }
            originalInstruction->originalInstructionImageWord = (VclPtrWord)cloneInstruction;
            cloneInstruction->optimizedInstructionImageWord = (VclPtrWord)originalInstruction;
        }
    }
    scheduleTable->clearClones();
    old->clearClones();
    workTablePtr = scheduleTable;
    optimizedTable = scheduleTable;
    if (scheduleTable->optimizerIndex >= 0)
        appliedOptimizationMask |= 1 << scheduleTable->optimizerIndex;
    if (scheduleTable->getLoopSize())
        hasLoopOptimization = 1;
    return true;
}
