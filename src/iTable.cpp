#include "os.h"
// iTable: work table construction and management (CIE group 24).
#include "workTable.h"
#include "instStream.h"
#include "Block.h"

ScheduleTable::ScheduleTable()
{
    rows = 0;
    moveListHead = 0;
    registerLockLists = 0;
    registerLockListCount = 0;
    postOptimizationPrepared = 0;
    ownerStream = 0;
    dispose();
}

ScheduleTable::~ScheduleTable()
{
    dispose();
}

bool ScheduleTable::init(unsigned char mode, int size)
{
    dispose();
    switch (mode) {
    case 2:
    case 3:
    case 5:
        size += 4;
        if (size < 4)
            size = 4;
        rows = new ScheduleRow[size];
        if (!rows)
            return false;
        rowCapacity = size;
        break;
    case 1:
    case 4:
        if (size) {
            reportError("can't ask for a table based itable (%d) for a synth/uta table...like.. perleaze make me an abstract base class and leave me alone!\n", size);
            return false;
        }
        break;
    default:
        reportError("iTable::Init() -%s for Init\n", codeModeStr(mode));
        return false;
    }
    rangeStart = 0;
    rangeEnd = 0;
    loopSize = 0;
    fallbackSchedulingActive = 0;
    nextInsertionSerial = 0;
    movePipeCapacityExceeded = 0;
    declaredRawLength = 0;
    minimumSafeLoopCount = 0;
    codeModeValue = mode;
    return true;
}

bool ScheduleTable::dispose()
{
    Instr *currentInstruction = pendingInstructions.head;
    while (currentInstruction) {
        Instr *next = currentInstruction->pendingLink.nextInstruction;
        if (currentInstruction->position.loopRow < 0)
            delete currentInstruction;
        currentInstruction = next;
    }
    postOptimizationPrepared = 0;
    if (!freeLockLists(registerLockLists)) {
        reportError("failed to dispose rll list\n");
        abortAfterError();
    }
    registerLockLists = 0;
    registerLockListCount = 0;
    registerUses.clear();
    delete[] rows;
    rows = 0;
    rowCapacity = 0;
    loopSize = 0;
    rangeStart = 0;
    rangeEnd = 0;
    mainLoopId = 0;
    optimizerIndex = -1;
    codeModeValue = 0;
    allocationMode = 0;
    rawSchedulingMode = 0;
    instructionCountsByPipe[2] = 0;
    dualPipelineInstructionCount = 0;
    vectorMoveCount = 0;
    oddPipeMoveCount = 0;
    instructionCountsByPipe[0] = 0;
    evenPipeMoveCount = 0;
    instructionCountsByPipe[1] = 0;
    moveListHead = 0;
    linearInstructions.reset();
    pendingInstructions.reset();
    completedInstructions.reset();
    pendingMoveCount = 0;
    nextInsertionSerial = 0;
    declaredRawLength = -1;
    ownerStream = 0;
    return true;
}

void ScheduleTable::dump()
{
    switch (codeModeValue) {
    case 0:
        break;
    case 1:
    case 4:
        for (Instr *currentInstruction = linearInstructions.head; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction)
            currentInstruction->dumpOperandDiagnostics(false);
        break;
    case 3:
    case 5:
        for (int dumpRowIndex = rangeStart; dumpRowIndex < rangeEnd; dumpRowIndex++) {
            ScheduleRow *row = rowAt(dumpRowIndex);
            if (!row)
                abortAfterError();
            row->refreshInstructionOperands(dumpRowIndex);
        }
        break;
    case 2: {
        int lsz = loopSize;
        if (!rows) {
            reportError("code mode mid - but no table!\n");
            abortAfterError();
        }
        if (!lsz && rangeEnd < 0) {
            ScheduleRow *row = rows;
            for (int rowIndex = rowCapacity - 1; rowIndex >= 0; rowIndex--, row++)
                if (row->entries[0].instruction || row->entries[1].instruction)
                    break;
        }
        for (int dumpRowIndex = rangeStart; dumpRowIndex < rangeEnd; dumpRowIndex++) {
            ScheduleRow *row = rowAt(dumpRowIndex);
            if (!row)
                abortAfterError();
            row->refreshInstructionOperands(dumpRowIndex);
        }
        for (Instr *currentInstruction = pendingInstructions.head; currentInstruction; currentInstruction = currentInstruction->pendingLink.nextInstruction)
            currentInstruction->dumpOperandDiagnostics(false);
        break;
    }
    default:
        reportError("unknown code mode for Dump()\n");
        abortAfterError();
    }
    registerUses.dumpRegisterUses();
}

const char *ScheduleTable::ownerName()
{
    if (!ownerStream)
        return "UNOWNED_TABLE";
    return ownerStream->streamName();
}

bool ScheduleTable::reserveEntry(int key, int row, int slot)
{
    bool even = !(key & 1);
    if (even) {
        reportError("can't reserve an entry with even key!\n");
        abortAfterError();
        return false;
    }
    Instr **pipelineEntries = ((InstructionOwnerTableView *)this)->getIE_ck(row);
    if (!pipelineEntries) {
        reportError("failed to reserve instruction!\n");
        return false;
    }
    if (pipelineEntries[slot]) {
        reportError("can't reserve instruction - something already there!\n");
        abortAfterError();
    }
    pipelineEntries[slot] = (Instr *)key;
    return true;
}

int ScheduleTable::getRawOrSyntheticLogicalLength()
{
    int len = -1;
    switch (codeModeValue) {
    case 3:
        len = declaredRawLength;
        break;
    case 1:
        reportError("called getlogical len on uta\n");
        abortAfterError();
        break;
    case 4:
        reportWarning("called getlogical on synth stream\n");
        len = 0;
        break;
    default:
        reportError("unknown code mode for logical len\n");
        abortAfterError();
    }
    return len;
}

int ScheduleTable::getLogicalLen()
{
    int len = 0;
    switch (codeModeValue) {
    case 1:
    case 3:
        len = logicalInstructionLength;
        break;
    case 4:
        break;
    default:
        reportError("unknown code mode for logical len\n");
        abortAfterError();
    }
    return len;
}

int ScheduleTable::getPhysicalLen()
{
    int len = -1;
    switch (codeModeValue) {
    case 3:
        if (!rows) {
            reportError("no table!\n");
            abortAfterError();
        }
        if (rangeEnd < rangeStart) {
            reportError("end before start\n");
            abortAfterError();
        }
        len = rangeEnd - rangeStart;
        break;
    case 1:
        reportError("called getphysical len on uta\n");
        abortAfterError();
        break;
    case 4:
        reportWarning("called getlogical on synth stream\n");
        len = 0;
        break;
    default:
        reportError("unknown code mode for logical len\n");
        abortAfterError();
    }
    if (len < 0) {
        reportError("getPhysicalLen() - would return 0 - halting\n");
        abortAfterError();
    }
    return len;
}

Instr *ScheduleTable::getConditionalBranch()
{
    Instr *secondInstruction = getBranch();
    if (secondInstruction && ((secondInstruction->branchFlags >> 2) & 1))
        return secondInstruction;
    return 0;
}

Instr *ScheduleTable::getBranch()
{
    Instr *secondInstruction = 0;
    switch (codeModeValue) {
    case 3:
        if (!rows) {
            reportWarning("called getBranch - but no table\n");
            return 0;
        }
        if (rangeEnd > rangeStart) {
            int branchRow = rangeEnd - 2;
            if (branchRow < rangeStart)
                return 0;
            secondInstruction = rowAt(branchRow)->getEntry(1);
        }
        break;
    case 1:
        secondInstruction = linearInstructions.tail;
        break;
    case 4:
        break;
    default: {
        const char *owner = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("bad code mode for getBranch %s for %s\n", codeModeStr(codeModeValue), owner);
        abortAfterError();
    }
    }
    InstructionStream *otherStream = ownerStream;
    if (!otherStream) {
        reportWarning("no owner set for getBranch\n");
        abortAfterError();
    } else if (secondInstruction) {
        if (secondInstruction->branchFlags) {
            if (secondInstruction->tildePrefix)
                return 0;
            else if (!otherStream->branchType) {
                const char *ownerName = otherStream->streamName();
                reportError("iTable::getBranch() for %s table has a branch at the end, but owning stream doesn't think it branches\n",
                              ownerName);
                dump();
                abortAfterError();
            }
        } else if (otherStream->branchType) {
            reportError("iTable::getBranch() - owner thinks it has a branch, but the instruction doesn't seem to be one\n");
            abortAfterError();
        }
    } else if (otherStream->branchType) {
        reportError("iTable::getBranch - owning stream thinks it has a branch - but there's no instruction here!\n");
        abortAfterError();
    }
    return (secondInstruction && secondInstruction->branchFlags) ? secondInstruction : 0;
}

bool unlinkFromLinearInstructions(Block *blk, Instr *n);
bool decrementInstructionPipelineCounts(Block *blk, Instr *in);

inline bool ScheduleTable::unlinkAndUnlog(Instr *instruction)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!unlinkFromLinearInstructions((Block *)this, instruction)) {
        reportError("logUnlinkFromLinear: failed to unlink instruction from linear\n");
        abortAfterError();
    }
    instruction->ownerTable = 0;
    if (!decrementInstructionPipelineCounts((Block *)this, instruction)) {
        reportError("failed to unlog instruction\n");
        return 0;
    }
    return 1;
}

bool ScheduleTable::removeBranch()
{
    Instr *secondInstruction = getBranch();
    if (!secondInstruction) {
        reportError("removeBranch - couldn't find a branch instruction\n");
        return 0;
    }
    if (!unlinkAndUnlog(secondInstruction)) {
        reportError("failed to take out the branch instruction\n");
        return 0;
    }
    delete secondInstruction;
    return 1;
}

int ScheduleTable::getTerminatingConditionalLoopId()
{
    int lid = -1;
    Instr *secondInstruction = getBranch();
    Instr *cloneInstruction = (secondInstruction && ((secondInstruction->branchFlags >> 2) & 1)) ? secondInstruction : 0;
    if (cloneInstruction)
        lid = cloneInstruction->position.loopIteration;
    if (loopSize) {
        if (lid < 0)
            reportError("getCondLid : no terminating conditional!\n");
    } else if (lid > 0) {
        reportError("getCondLid : terminating conditional had lid>0\n");
    }
    return lid;
}

bool ScheduleTable::takeOut(Instr *instruction)
{
    InstructionPosition *position = &instruction->position;
    ScheduleRow *row = &rows[position->loopRow];
    row->entries[position->pipelineIndex].instruction = (Instr *)instruction->replacedInstructionWord;
    instruction->clearSlot();
    return 1;
}

bool ScheduleTable::putIn(Instr *instruction)
{
    InstructionPosition *position = &instruction->position;
    ScheduleRow *row = &rows[position->loopRow];
    row->entries[position->pipelineIndex].instruction = instruction;
    return 1;
}

bool cloneAppend(ScheduleTable *from, ScheduleTable *scheduleTable, int lidS, int lidE,
                 bool includeBranches, bool adjustCloneAliasLoopId, bool includeNops)
{
    if (lidE > lidS) {
        reportError("CloneAppend - lid_e>lid_s\n");
        abortAfterError();
    }
    if (!scheduleTable) {
        reportError("CloneAppend - null input pointer\n");
        return 0;
    }
    if (!scheduleTable->rows) {
        reportError("clone append - source table has no attached table\n");
        return 0;
    }
    if (!from->rows) {
        reportError("clone append - destination table has no attached table\n");
        return 0;
    }
    int appendedRowCount = from->rangeEnd - from->rangeStart;
    int need = appendedRowCount + scheduleTable->rangeEnd;
    if (need > scheduleTable->rowCapacity && !scheduleTable->grow(need + 0x20)) {
        reportError("uh-oh, failed to grow a table\n");
        return 0;
    }
    int rowIndex, copySourceRow;
    for (rowIndex = scheduleTable->rangeEnd, copySourceRow = from->rangeStart; copySourceRow != from->rangeEnd; copySourceRow++, rowIndex++) {
        ScheduleRow *sourceRow = &from->rows[copySourceRow];
        ScheduleRow *destinationRow = &scheduleTable->rows[rowIndex];
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *cloneInstruction;
            Instr *instruction = sourceRow->getEntry(entryIndex);
            cloneInstruction = 0;
            if (destinationRow->getEntry(entryIndex)) {
                reportError("CloneAppend: would overwrite inst in target\n");
                return 0;
            }
            if (instruction) {
                if (!instruction->opcodeInfo) {
                    reportError("no II in inst\n");
                    from->dump();
                }
                if (!instruction->isNop() || includeNops) {
                    int lid = instruction->position.loopIteration;
                    if (lid <= lidS && lid >= lidE) {
                        unsigned char branchFlags = instruction->branchFlags;
                        if ((branchFlags && includeBranches) || !instruction->getBranchType()) {
                            cloneInstruction = instruction->clone();
                            if (!cloneInstruction) {
                                reportError("failed to clone instruction\n");
                                return 0;
                            }
                            cloneInstruction->cloneImage = 0;
                            instruction->cloneImage = 0;
                            if (adjustCloneAliasLoopId)
                                cloneInstruction->expectedAliasLoopId = lidS - lid;
                        }
                    }
                }
            }
            destinationRow->entries[entryIndex].instruction = cloneInstruction;
            if (cloneInstruction) {
                InstructionPosition pos;
                cloneInstruction->setOwner((InstructionOwnerTableView *)scheduleTable);
                pos.setRow(rowIndex);
                pos.pipelineIndex = entryIndex;
                InstructionPosition *loc = &cloneInstruction->position;
                cloneInstruction->position.linearRow = pos.linearRow;
                loc->loopRow = pos.loopRow;
                loc->loopIteration = pos.loopIteration;
                loc->pipelineIndex = pos.pipelineIndex;
            }
        }
    }
    scheduleTable->rangeEnd += appendedRowCount;
    return 1;
}

void ScheduleTable::dumpSynthetics()
{
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        if (instruction->syntheticKind)
            instruction->dumpOperandDiagnostics(false);
}

bool ScheduleTable::clearClones()
{
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        instruction->cloneImage = 0;
    return 1;
}

int ScheduleTable::getTableRawMode()
{
    if (codeModeValue == 3 || codeModeValue == 5)
        return rawSchedulingMode;
    return 0;
}

bool ScheduleTable::setTableRawMode(unsigned char mode)
{
    if (codeModeValue != 3 && codeModeValue != 5 && mode) {
        int currentCodeMode = codeModeValue;
        reportError("failed to set vsm mode - because code mode isn't vsm..mode=%d cm=%d\n", mode, currentCodeMode);
        abortAfterError();
        return 0;
    }
    rawSchedulingMode = mode;
    return 1;
}

Instr *earliestInsertionInstruction;

void trackEarliestInsertion(Instr *instruction)
{
    if (instruction) {
        if (!earliestInsertionInstruction || instruction->insertionSerial < earliestInsertionInstruction->insertionSerial)
            earliestInsertionInstruction = instruction;
    }
}

bool ScheduleTable::resetFlowStalls()
{
    declaredRawLength = -1;
    if (rows && rangeStart >= 0) {
        for (int rowIndex = rangeStart; rowIndex < rangeEnd; rowIndex++)
            if (!rows[rowIndex].clearStalls()) {
                reportError("failed to flow reset ientry\n");
                return 0;
            }
    }
    return 1;
}

bool ScheduleTable::hasSafeTrailingInstructions()
{
    if (rangeStart < 0)
        return 0;
    if (rangeEnd <= 0)
        return 1;
    ScheduleRow *row = rowAt(rangeEnd - 1);
    if (!row)
        return 0;
    Instr *instruction = row->entries[1].instruction;
    if (instruction) {
        if (instruction->ramAccessType || instruction->branchFlags)
            return 0;
        int opcodeId = getOpcodeId(instruction->opcodeInfo);
        if (opcodeId >= 165 && opcodeId <= 167)
            return 0;
    }
    row = rowAt(rangeEnd - 2);
    if (!row)
        return 1;
    instruction = row->entries[1].instruction;
    if (instruction && getOpcodeId(instruction->opcodeInfo) == 165)
        return 0;
    return 1;
}

Instr **InstructionOwnerTableView::getIE_ck(int rowIndex)
{
    ScheduleTable *table = (ScheduleTable *)this;
    if (!table->rows) {
        reportError("checkie: table not set\n");
        return 0;
    }
    if (rowIndex < 0) {
        reportError("checkie() - rl<0\n");
        return 0;
    }
    if (table->loopSize) {
        if (table->rangeStart) {
            reportError("getIE_ck - iStart not zero for loop table\n");
            abortAfterError();
        }
        if (rowIndex >= table->loopSize)
            rowIndex %= table->loopSize;
    }
    if (rowIndex >= table->rowCapacity) {
        reportError("checkie() - rl>=m_MaxSize %d,%d\n", rowIndex, table->rowCapacity);
        return 0;
    }
    return (Instr **)&table->rows[rowIndex];
}

Instr *ScheduleTable::getInstr(int rowIndex, int pipelineIndex)
{
    Instr **pipelineEntries = ((InstructionOwnerTableView *)this)->getIE_ck(rowIndex);
    if (!pipelineEntries)
        return 0;
    if (pipelineIndex < 0 || pipelineIndex >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return pipelineEntries[pipelineIndex];
}

static inline Instr *getPipeInstr(Instr **pipelineEntries, int pipelineIndex)
{
    if (!pipelineEntries)
        return 0;
    if (pipelineIndex < 0 || pipelineIndex >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return pipelineEntries[pipelineIndex];
}

Instr *ScheduleTable::getInstrInRange(int rowIndex, int pipeIndex)
{
    if (rowIndex < rangeStart || rowIndex >= rangeEnd)
        return 0;
    Instr **pipelineEntries = ((InstructionOwnerTableView *)this)->getIE_ck(rowIndex);
    return getPipeInstr(pipelineEntries, pipeIndex);
}

const char *codeModeName(int mode)
{
    return codeModeStr(mode);
}

const char *rawModeName(int mode)
{
    switch (mode) {
    case 0: return "RAW_MODE_BAD";
    case 1: return "RAW_MODE_VCL_FREE";
    case 2: return "RAW_MODE_VCL_FIXED";
    case 3: return "RAW_MODE_USER_FREE";
    case 4: return "RAW_MODE_USER_FIXED";
    }
    return "RAW_MODE_????";
}

const char *allocModeName(unsigned int mode)
{
    switch (mode) {
    case 0: return "ALLOC_MODE_BAD";
    case 1: return "ALLOC_MODE_NONE";
    case 2: return "ALLOC_MODE_X";
    case 3: return "ALLOC_MODE_DONE";
    }
    return "ALLOC_MODE_????";
}

int ScheduleTable::getPipeCount(int pipeIndex)
{
    if (pipeIndex < 0 || pipeIndex > 2) {
        reportError("get pipe count - bad pipe\n");
        abortAfterError();
    }
    return instructionCountsByPipe[pipeIndex];
}

bool logInstr(Block *block, Instr *instruction)
{
    if (instruction->permittedPipelineMask == 3)
        block->dualPipelineInstructionCount++;
    else {
        int pipelineCount = 0;
        if (instruction->permittedPipelineMask & 1) {
            block->instructionCountsByPipe[0]++;
            pipelineCount = 1;
        }
        if (instruction->permittedPipelineMask & 2) {
            pipelineCount++;
            block->instructionCountsByPipe[1]++;
        }
        if (pipelineCount != 1) {
            reportError("nad too many / too few\n");
            abortAfterError();
        }
    }
    block->instructionCountsByPipe[2]++;
    return 1;
}

bool decrementInstructionPipelineCounts(Block *block, Instr *instruction)
{
    if (instruction->permittedPipelineMask == 3)
        block->dualPipelineInstructionCount--;
    else {
        int pipelineCount = 0;
        if (instruction->permittedPipelineMask & 1) {
            block->instructionCountsByPipe[0]--;
            pipelineCount = 1;
        }
        if (instruction->permittedPipelineMask & 2) {
            pipelineCount++;
            block->instructionCountsByPipe[1]--;
        }
        if (pipelineCount != 1) {
            reportError("nad too many\n");
            abortAfterError();
        }
    }
    if (--block->instructionCountsByPipe[2] < 0) {
        reportError("-ve icount\n");
        return 0;
    }
    return 1;
}

void ScheduleTable::resetCounts()
{
    instructionCountsByPipe[2] = 0;
    dualPipelineInstructionCount = 0;
    vectorMoveCount = 0;
    oddPipeMoveCount = 0;
    instructionCountsByPipe[0] = 0;
    evenPipeMoveCount = 0;
    instructionCountsByPipe[1] = 0;
}

int ScheduleTable::getCurrentDoneSize()
{
    if (loopSize)
        return loopSize;
    int size = 0;
    for (Instr *instruction = completedInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        int row = instruction->position.linearRow;
        if (row < 0) {
            reportError("getCurrentDoneSize()\n");
            return -1;
        }
        short outputConstraint = instruction->externalOutputConstraint;
        if (outputConstraint < instruction->externalOutputPairedConstraint)
            outputConstraint = instruction->externalOutputPairedConstraint;
        int minToEnd = outputConstraint;
        if (minToEnd < 0) {
            reportError("getCurrentDoneSize() - minToEnd was bad\n");
            return -1;
        }
        if (instruction->branchFlags && minToEnd <= 0)
            minToEnd = 1;
        if (row + minToEnd + 1 > size)
            size = row + minToEnd + 1;
    }
    if (size < externalMinimumLength)
        size = externalMinimumLength;
    return size;
}

bool ScheduleTable::getCurrentLidStats(int *maxLid, int *maxBranchLid, int *minSfx1, int *minSfx2)
{
    Instr *instruction = completedInstructions.head;
    *maxLid = -1;
    *maxBranchLid = -1;
    *minSfx1 = 30000;
    *minSfx2 = 30000;
    for (; instruction; instruction = instruction->pendingLink.nextInstruction) {
        int lid = instruction->position.loopIteration;
        if (lid > *maxLid)
            *maxLid = lid;
        switch ((unsigned char)instruction->schedulerCategory) {
        case 0:
            break;
        case 1:
            if (lid < *minSfx1)
                *minSfx1 = lid;
            break;
        case 2:
            if (lid < *minSfx2)
                *minSfx2 = lid;
            break;
        default:
            reportError("getCurrentLidStats - bad sfx\n");
            return 0;
        }
        if (instruction->branchFlags && lid > *maxBranchLid)
            *maxBranchLid = lid;
    }
    if (*minSfx1 >= 30000)
        *minSfx1 = -1;
    if (*minSfx2 >= 30000)
        *minSfx2 = -1;
    return 1;
}

bool ScheduleTable::grow(int rowIndex)
{
    if (!rows) {
        reportError("can't grow.. there's nothing here\n");
        return 0;
    }
    if (rowIndex <= rowCapacity)
        return 1;
    ScheduleRow *workRow = new ScheduleRow[rowIndex];
    if (!workRow) {
        reportError("failed to get new tab\n");
        return 0;
    }
    for (int transferredRowIndex = rangeStart; transferredRowIndex < rangeEnd; transferredRowIndex++) {
        workRow[transferredRowIndex] = rows[transferredRowIndex];
        rows[transferredRowIndex].clearEntries();
    }
    delete[] rows;
    rows = workRow;
    rowCapacity = rowIndex;
    return 1;
}

bool ScheduleTable::hasEmptyRange()
{
    if (!rows)
        return 1;
    return rangeEnd <= rangeStart;
}

bool ScheduleTable::setRange()
{
    if (!rows) {
        reportError("setRange - no table!\n");
        return 0;
    }
    rangeStart = 0;
    rangeEnd = -1;
    loopSize = 0;
    return 1;
}

bool ScheduleTable::setRange(int len)
{
    if (!rows) {
        reportError("setRange - no table!\n");
        return 0;
    }
    rangeStart = 0;
    rangeEnd = len;
    loopSize = len;
    return 1;
}

bool ScheduleTable::setRange(int startRow, int endRow)
{
    if (!rows) {
        reportError("setRange - no table!\n");
        return 0;
    }
    rangeStart = startRow;
    rangeEnd = endRow;
    loopSize = 0;
    if (startRow < 0) {
        reportError("s before 0\n");
        abortAfterError();
        return 0;
    }
    if (endRow < startRow) {
        reportError("setRange : e<s before [%d,%d]\n", startRow, endRow);
        abortAfterError();
        return 0;
    }
    if (endRow >= rowCapacity) {
        reportError("e after end\n");
        abortAfterError();
        return 0;
    }
    return 1;
}

bool ScheduleTable::setSkip(int row, char skip)
{
    if (row < rangeStart) {
        reportError("setskip - before start\n");
        return 0;
    }
    if (row >= rangeEnd) {
        reportError("setskip >= end\n");
        return 0;
    }
    ScheduleRow *targetRow = rowAt(row);
    if (!targetRow) {
        reportError("failed to get ie in setSkip\n");
        return 0;
    }
    targetRow->skip = skip;
    return 1;
}

int ScheduleTable::getRangeStart()
{
    if (!rows) {
        const char *modeName = codeModeStr(codeModeValue);
        const char *ownerName = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("getStart called .. no table for %s mode %s\n", ownerName, modeName);
        exitAfterCleanup(-1);
    }
    return rangeStart;
}

int ScheduleTable::getLoopSize()
{
    return loopSize;
}

int ScheduleTable::getRangeEnd()
{
    if (!rows) {
        const char *ownerName = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("getEnd called .. no table for %s\n", ownerName);
        reportError("getEnd called .. no table\n");
        exitAfterCleanup(-1);
    }
    return rangeEnd;
}

bool ScheduleTable::changeMode(unsigned char mode)
{
    int cur = codeModeValue;
    if (mode == (unsigned char)cur) {
        reportError("change mode to same!\n");
        abortAfterError();
        return 0;
    }
    switch (cur) {
    case 0:
        reportError("can't change mode from bad\n");
        abortAfterError();
        return 0;
    case 1:
        reportError("can't change uta->\n");
        abortAfterError();
        return 0;
    case 2:
        if (mode != 3) {
            reportError("can only go mid->raw\n");
            abortAfterError();
            return 0;
        }
        break;
    case 3:
        reportError("can't change from raw to %s\n", codeModeStr(mode));
        abortAfterError();
        return 0;
    case 4:
        reportError("can't change from synth\n");
        abortAfterError();
        break;
    case 5:
        if (mode != 3) {
            reportError("can only go preexp->raw\n");
            abortAfterError();
            return 0;
        }
        break;
    default:
        reportError("bad code mode for change mode\n");
        abortAfterError();
        break;
    }
    codeModeValue = mode;
    return 1;
}

bool insertLinearInstructionAfter(Block *blk, Instr *after, Instr *n);

bool ScheduleTable::logAddLinear_After(Instr *after, Instr *insertedInstruction)
{
    if (!insertedInstruction) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!insertedInstruction->setOwner((InstructionOwnerTableView *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!insertLinearInstructionAfter((Block *)this, after, insertedInstruction)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, insertedInstruction)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool prependLinearInstruction(Block *blk, Instr *n);

bool ScheduleTable::logAddLinear_Head(Instr *headInstruction)
{
    if (!headInstruction) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!headInstruction->setOwner((InstructionOwnerTableView *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!prependLinearInstruction((Block *)this, headInstruction)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, headInstruction)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool appendLinearInstruction(Block *blk, Instr *n);

bool ScheduleTable::logAddLinear_Tail(Instr *tailInstruction)
{
    if (!tailInstruction) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!tailInstruction->setOwner((InstructionOwnerTableView *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!appendLinearInstruction((Block *)this, tailInstruction)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, tailInstruction)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool ScheduleTable::makeLinearList()
{
    clearLinear();
    if (rangeStart < 0 || !rows) {
        reportError("iTable::makeLinearList - table not ready\n");
        return 0;
    }
    for (int rowIndex = rangeStart; rowIndex < rangeEnd; rowIndex++) {
        Instr **pipelineEntries = ((InstructionOwnerTableView *)this)->getIE_ck(rowIndex);
        if (!pipelineEntries) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int pipelineIndex = 0; pipelineIndex != 2; pipelineIndex++) {
            Instr *instruction = pipelineEntries[pipelineIndex];
            if (instruction && !logAddLinear_Tail(instruction)) {
                reportError("iTable - failed to add an instruction\n");
                instruction->dumpInstructionContext();
                return 0;
            }
        }
    }
    return 1;
}

bool ScheduleTable::logUnlinkFromLinear(Instr *instruction)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!unlinkFromLinearInstructions((Block *)this, instruction)) {
        reportError("logUnlinkFromLinear: failed to unlink instruction from linear\n");
        abortAfterError();
    }
    instruction->ownerTable = 0;
    if (!decrementInstructionPipelineCounts((Block *)this, instruction)) {
        reportError("failed to unlog instruction\n");
        return 0;
    }
    return 1;
}

bool ScheduleTable::relinkRawIt()
{
    if (!rows) {
        reportError("relinkRawIt() -no table set\n");
        return 0;
    }
    clearLinear();
    for (int rowIndex = rangeStart; rowIndex < rangeEnd; rowIndex++) {
        ScheduleRow *row = rowAt(rowIndex);
        if (!row) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *instruction = row->entries[entryIndex].instruction;
            if (instruction) {
                instruction->linearLink.previousInstruction = 0;
                instruction->linearLink.nextInstruction = 0;
                if (!logAddLinear_Tail(instruction)) {
                    reportError("failed to add linear tail\n");
                    return 0;
                }
            }
        }
    }
    return 1;
}

bool ScheduleTable::lockLine(InstructionPosition *line)
{
    return 1;
}

bool ScheduleTable::unlockLine(InstructionPosition *line)
{
    return 1;
}

ScheduleTable *ScheduleTable::localClone()
{
    if (codeModeValue != 1) {
        reportError("table to be locally cloned isn't uta\n");
        return 0;
    }
    ScheduleTable *scheduleTable = new ScheduleTable;
    if (!scheduleTable) {
        reportError("failed to get new table\n");
        return 0;
    }
    scheduleTable->init(1, 0);
    scheduleTable->minimumSafeLoopCount = minimumSafeLoopCount;
    scheduleTable->statistics.copyCountersFrom(&statistics);
    scheduleTable->streamDependencyLength = streamDependencyLength;
    scheduleTable->streamLogicalLength = streamLogicalLength;
    scheduleTable->logicalInstructionLength = logicalInstructionLength;
    scheduleTable->externalMinimumLength = externalMinimumLength;
    unsigned int allocationModeValue = (unsigned char)allocationMode;
    scheduleTable->allocationMode = allocationModeValue;
    scheduleTable->nextInsertionSerial = 0;
    scheduleTable->ownerStream = ownerStream;
    RegisterUseList &regs = registerUses;
    for (RegisterUse *recordedUse = regs.head; recordedUse; recordedUse = recordedUse->next) {
        RegisterUse *cloned = recordedUse->Clone();
        if (!scheduleTable->registerUses.append(cloned)) {
            reportError("failed to add cloned regref to list\n");
            delete recordedUse;
            return 0;
        }
    }
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        Instr *cloneInstruction = instruction->clone();
        for (int operandIndex = 0; operandIndex != cloneInstruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegisterEntry *registerEntry = registerOperand(operand, 1);
            if (registerEntry) {
            int mask = operand->componentMask & ~operand->externalPartMask;
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                int bit = 1 << partIndex;
                if (mask & bit) {
                OperandLocation *operandLocation = operand->parts[partIndex].operandLocations.head;
                if (!operandLocation || operandLocation->next) {
                    reportError("more or less than one oploc for non-extern input\n");
                    return 0;
                }
                Instr *setter = operandLocation->instruction;
                if (!setter) {
                    reportError("could not get setting op for setter\n");
                    return 0;
                }
                InstructionStream *stream = setter->getOwningStream();
                if (stream != ownerStream) {
                    reportError("owner of setter not us %p vs %p\n", setter->ownerTable, this, stream);
                    return 0;
                }
                if (!setter->cloneImage) {
                    reportError("qInstruction - setter had null work clone set\n");
                    return 0;
                }
                Instr *writerInstruction = setter->cloneImage;
                int p1 = operandLocation->componentIndex;
                int p0 = operandLocation->operandIndex;
                if (!linkOperandParts(writerInstruction, p0, p1, cloneInstruction, operandIndex, partIndex)) {
                    reportError("failed to link local oparts\n");
                    return 0;
                }
                }
            }
            }
        }
        if (!scheduleTable->logAddLinear_Tail(cloneInstruction)) {
            reportError("iTable::qInstruction - failed to add to linear list tail\n");
            return 0;
        }
    }
    scheduleTable->clearClones();
    clearClones();
    return scheduleTable;
}

bool ScheduleTable::replaceInstruction(Instr *old, Instr *newInstruction)
{
    if (old->ownerTable != (InstructionOwnerTableView *)this) {
        reportError("old instruction isn't in table\n");
        return 0;
    }
    InstructionOwnerTableView *own = newInstruction->ownerTable;
    if (own) {
        reportError("replaceInstruction - new instruction already has owner\n");
        return 0;
    }
    Instr *prev = old->linearLink.previousInstruction;
    if (!logUnlinkFromLinear(old)) {
        reportError("failed to take out old instruction from linear\n");
        return 0;
    }
    newInstruction->linearOrderIndex = old->linearOrderIndex;
    newInstruction->instructionSerial = old->instructionSerial;
    if (isAbstract()) {
        if (newInstruction->isConcreteMove()) {
            reportError("trying to place a resolved move into abstract table\n");
            return 0;
        }
        if (!logAddLinear_After(prev, newInstruction)) {
            reportError("failed to log new instruction \n");
            return 0;
        }
    } else {
        if (newInstruction->isAbstractMove()) {
            reportError("trying to replace an abstract move into a table\n");
            return 0;
        }
        if (!logAddLinear_After(prev, newInstruction)) {
            reportError("failed to log new instruction \n");
            return 0;
        }
        InstructionPosition pos;
        InstructionPosition *instructionPosition = &old->position;
        pos.linearRow = instructionPosition->linearRow;
        pos.loopRow = instructionPosition->loopRow;
        pos.loopIteration = instructionPosition->loopIteration;
        pos.pipelineIndex = instructionPosition->pipelineIndex;
        InstructionPosition *position = &newInstruction->position;
        position->linearRow = old->position.linearRow;
        position->loopRow = pos.loopRow;
        position->loopIteration = pos.loopIteration;
        position->pipelineIndex = pos.pipelineIndex;
        if (!takeOut(old)) {
            reportError("failed to take out old instruction\n");
            return 0;
        }
        ScheduleRow *row = &rows[position->loopRow];
        row->entries[position->pipelineIndex].instruction = newInstruction;
    }
    return 1;
}

bool ScheduleTable::destroyInstruction(Instr *&removedInstruction)
{
    if (!logUnlinkFromLinear(removedInstruction)) {
        reportError("destroyInstruction -  failed to unlink\n");
        return 0;
    }
    if (rows) {
        InstructionPosition *position = &removedInstruction->position;
        int row = position->loopRow;
        int pipe = position->pipelineIndex;
        Instr **pipelineEntries = ((InstructionOwnerTableView *)this)->getIE_ck(row);
        if (!pipelineEntries) {
            reportError("logUnlinkFromLinear: failed to get IE where insn ought to be (line %d.%d)\n", row, pipe);
            return 0;
        }
        if (pipelineEntries[pipe] != removedInstruction) {
            reportError("logUnlinkFromLinear: instruction wasn't where I thought it'd be!\n");
            return 0;
        }
        pipelineEntries[pipe] = 0;
    }
    removedInstruction = 0;
    return 1;
}
