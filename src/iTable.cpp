#include "os.h"
// iTable: work table construction and management (CIE group 24).
#include "workTable.h"
#include "instStream.h"
#include "Block.h"

workTable::workTable()
{
    rows = 0;
    moveListHead = 0;
    registerLockLists = 0;
    registerLockListCount = 0;
    postOptimizationPrepared = 0;
    ownerStream = 0;
    dispose();
}

workTable::~workTable()
{
    dispose();
}

bool workTable::init(unsigned char mode, int size)
{
    dispose();
    switch (mode) {
    case 2:
    case 3:
    case 5:
        size += 4;
        if (size < 4)
            size = 4;
        rows = new wtRow[size];
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
    unk_ad = 0;
    declaredRawLength = 0;
    minimumSafeLoopCount = 0;
    codeModeValue = mode;
    return true;
}

bool workTable::dispose()
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

void workTable::dump()
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
        for (int i = rangeStart; i < rangeEnd; i++) {
            wtRow *row = rowAt(i);
            if (!row)
                abortAfterError();
            row->refreshInstructionOperands(i);
        }
        break;
    case 2: {
        int lsz = loopSize;
        if (!rows) {
            reportError("code mode mid - but no table!\n");
            abortAfterError();
        }
        if (!lsz && rangeEnd < 0) {
            wtRow *row = rows;
            for (int n = rowCapacity - 1; n >= 0; n--, row++)
                if (row->entries[0].instruction || row->entries[1].instruction)
                    break;
        }
        for (int i = rangeStart; i < rangeEnd; i++) {
            wtRow *row = rowAt(i);
            if (!row)
                abortAfterError();
            row->refreshInstructionOperands(i);
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

const char *workTable::ownerName()
{
    if (!ownerStream)
        return "UNOWNED_TABLE";
    return ownerStream->streamName();
}

bool workTable::reserveEntry(int key, int row, int slot)
{
    bool even = !(key & 1);
    if (even) {
        reportError("can't reserve an entry with even key!\n");
        abortAfterError();
        return false;
    }
    Instr **pipelineEntries = ((cls_unk30 *)this)->getIE_ck(row);
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

int workTable::getRawOrSyntheticLogicalLength()
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

int workTable::getLogicalLen()
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

int workTable::getPhysicalLen()
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

Instr *workTable::getConditionalBranch()
{
    Instr *secondInstruction = getBranch();
    if (secondInstruction && ((secondInstruction->branchFlags >> 2) & 1))
        return secondInstruction;
    return 0;
}

Instr *workTable::getBranch()
{
    Instr *secondInstruction = 0;
    switch (codeModeValue) {
    case 3:
        if (!rows) {
            reportWarning("called getBranch - but no table\n");
            return 0;
        }
        if (rangeEnd > rangeStart) {
            int r = rangeEnd - 2;
            if (r < rangeStart)
                return 0;
            secondInstruction = rowAt(r)->getEntry(1);
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
    instStream *otherStream = ownerStream;
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

bool UnlinkFromLinear(Block *blk, Instr *n);
bool decrementInstructionPipelineCounts(Block *blk, Instr *in);

inline bool workTable::unlinkAndUnlog(Instr *instruction)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!UnlinkFromLinear((Block *)this, instruction)) {
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

bool workTable::removeBranch()
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

int workTable::getCondLid()
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

bool workTable::takeOut(Instr *instruction)
{
    InstructionPosition *position = &instruction->position;
    wtRow *row = &rows[position->loopRow];
    row->entries[position->pipelineIndex].instruction = (Instr *)instruction->replacedInstructionWord;
    instruction->clearSlot();
    return 1;
}

bool workTable::putIn(Instr *instruction)
{
    InstructionPosition *position = &instruction->position;
    wtRow *row = &rows[position->loopRow];
    row->entries[position->pipelineIndex].instruction = instruction;
    return 1;
}

bool cloneAppend(workTable *from, workTable *to, int lidS, int lidE, bool a5, bool a6, bool a7)
{
    if (lidE > lidS) {
        reportError("CloneAppend - lid_e>lid_s\n");
        abortAfterError();
    }
    if (!to) {
        reportError("CloneAppend - null input pointer\n");
        return 0;
    }
    if (!to->rows) {
        reportError("clone append - source table has no attached table\n");
        return 0;
    }
    if (!from->rows) {
        reportError("clone append - destination table has no attached table\n");
        return 0;
    }
    int n = from->rangeEnd - from->rangeStart;
    int need = n + to->rangeEnd;
    if (need > to->rowCapacity && !to->grow(need + 0x20)) {
        reportError("uh-oh, failed to grow a table\n");
        return 0;
    }
    int j, i;
    for (j = to->rangeEnd, i = from->rangeStart; i != from->rangeEnd; i++, j++) {
        wtRow *sourceRow = &from->rows[i];
        wtRow *destinationRow = &to->rows[j];
        for (int p = 0; p != 2; p++) {
            Instr *cloneInstruction;
            Instr *instruction = sourceRow->getEntry(p);
            cloneInstruction = 0;
            if (destinationRow->getEntry(p)) {
                reportError("CloneAppend: would overwrite inst in target\n");
                return 0;
            }
            if (instruction) {
                if (!instruction->opcodeInfo) {
                    reportError("no II in inst\n");
                    from->dump();
                }
                if (!instruction->isNop() || a7) {
                    int lid = instruction->position.loopIteration;
                    if (lid <= lidS && lid >= lidE) {
                        unsigned char f = instruction->branchFlags;
                        if ((f && a5) || !instruction->getBranchType()) {
                            cloneInstruction = instruction->clone();
                            if (!cloneInstruction) {
                                reportError("failed to clone instruction\n");
                                return 0;
                            }
                            cloneInstruction->cloneImage = 0;
                            instruction->cloneImage = 0;
                            if (a6)
                                cloneInstruction->expectedAliasLoopId = lidS - lid;
                        }
                    }
                }
            }
            destinationRow->entries[p].instruction = cloneInstruction;
            if (cloneInstruction) {
                InstructionPosition pos;
                cloneInstruction->setOwner((cls_unk30 *)to);
                pos.setRow(j);
                pos.pipelineIndex = p;
                InstructionPosition *loc = &cloneInstruction->position;
                cloneInstruction->position.linearRow = pos.linearRow;
                loc->loopRow = pos.loopRow;
                loc->loopIteration = pos.loopIteration;
                loc->pipelineIndex = pos.pipelineIndex;
            }
        }
    }
    to->rangeEnd += n;
    return 1;
}

void workTable::dumpSynthetics()
{
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        if (instruction->syntheticKind)
            instruction->dumpOperandDiagnostics(false);
}

bool workTable::clearClones()
{
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        instruction->cloneImage = 0;
    return 1;
}

int workTable::getTableRawMode()
{
    if (codeModeValue == 3 || codeModeValue == 5)
        return rawSchedulingMode;
    return 0;
}

bool workTable::setTableRawMode(unsigned char mode)
{
    if (codeModeValue != 3 && codeModeValue != 5 && mode) {
        int cm = codeModeValue;
        reportError("failed to set vsm mode - because code mode isn't vsm..mode=%d cm=%d\n", mode, cm);
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

bool workTable::resetFlowStalls()
{
    declaredRawLength = -1;
    if (rows && rangeStart >= 0) {
        for (int i = rangeStart; i < rangeEnd; i++)
            if (!rows[i].clearStalls()) {
                reportError("failed to flow reset ientry\n");
                return 0;
            }
    }
    return 1;
}

bool workTable::hasSafeTrailingInstructions()
{
    if (rangeStart < 0)
        return 0;
    if (rangeEnd <= 0)
        return 1;
    wtRow *row = rowAt(rangeEnd - 1);
    if (!row)
        return 0;
    Instr *instruction = row->entries[1].instruction;
    if (instruction) {
        if (instruction->ramAccessType || instruction->branchFlags)
            return 0;
        int op = getOpcodeId(instruction->opcodeInfo);
        if (op >= 165 && op <= 167)
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

Instr **cls_unk30::getIE_ck(int rl)
{
    workTable *table = (workTable *)this;
    if (!table->rows) {
        reportError("checkie: table not set\n");
        return 0;
    }
    if (rl < 0) {
        reportError("checkie() - rl<0\n");
        return 0;
    }
    if (table->loopSize) {
        if (table->rangeStart) {
            reportError("getIE_ck - iStart not zero for loop table\n");
            abortAfterError();
        }
        if (rl >= table->loopSize)
            rl %= table->loopSize;
    }
    if (rl >= table->rowCapacity) {
        reportError("checkie() - rl>=m_MaxSize %d,%d\n", rl, table->rowCapacity);
        return 0;
    }
    return (Instr **)&table->rows[rl];
}

Instr *workTable::getInstr(int rl, int p)
{
    Instr **pipelineEntries = ((cls_unk30 *)this)->getIE_ck(rl);
    if (!pipelineEntries)
        return 0;
    if (p < 0 || p >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return pipelineEntries[p];
}

static inline Instr *getPipeInstr(Instr **pipelineEntries, int p)
{
    if (!pipelineEntries)
        return 0;
    if (p < 0 || p >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return pipelineEntries[p];
}

Instr *workTable::getInstrInRange(int rl, int p)
{
    if (rl < rangeStart || rl >= rangeEnd)
        return 0;
    Instr **pipelineEntries = ((cls_unk30 *)this)->getIE_ck(rl);
    return getPipeInstr(pipelineEntries, p);
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

int workTable::getPipeCount(int p)
{
    if (p < 0 || p > 2) {
        reportError("get pipe count - bad pipe\n");
        abortAfterError();
    }
    return instructionCountsByPipe[p];
}

bool logInstr(Block *blk, Instr *instruction)
{
    if (instruction->permittedPipelineMask == 3)
        blk->dualPipelineInstructionCount++;
    else {
        int n = 0;
        if (instruction->permittedPipelineMask & 1) {
            blk->instructionCountsByPipe[0]++;
            n = 1;
        }
        if (instruction->permittedPipelineMask & 2) {
            n++;
            blk->instructionCountsByPipe[1]++;
        }
        if (n != 1) {
            reportError("nad too many / too few\n");
            abortAfterError();
        }
    }
    blk->instructionCountsByPipe[2]++;
    return 1;
}

bool decrementInstructionPipelineCounts(Block *blk, Instr *instruction)
{
    if (instruction->permittedPipelineMask == 3)
        blk->dualPipelineInstructionCount--;
    else {
        int n = 0;
        if (instruction->permittedPipelineMask & 1) {
            blk->instructionCountsByPipe[0]--;
            n = 1;
        }
        if (instruction->permittedPipelineMask & 2) {
            n++;
            blk->instructionCountsByPipe[1]--;
        }
        if (n != 1) {
            reportError("nad too many\n");
            abortAfterError();
        }
    }
    if (--blk->instructionCountsByPipe[2] < 0) {
        reportError("-ve icount\n");
        return 0;
    }
    return 1;
}

void workTable::resetCounts()
{
    instructionCountsByPipe[2] = 0;
    dualPipelineInstructionCount = 0;
    vectorMoveCount = 0;
    oddPipeMoveCount = 0;
    instructionCountsByPipe[0] = 0;
    evenPipeMoveCount = 0;
    instructionCountsByPipe[1] = 0;
}

int workTable::getCurrentDoneSize()
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
        short m = instruction->externalOutputConstraint;
        if (m < instruction->externalOutputPairedConstraint)
            m = instruction->externalOutputPairedConstraint;
        int minToEnd = m;
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

bool workTable::getCurrentLidStats(int *maxLid, int *maxBranchLid, int *minSfx1, int *minSfx2)
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

bool workTable::grow(int n)
{
    if (!rows) {
        reportError("can't grow.. there's nothing here\n");
        return 0;
    }
    if (n <= rowCapacity)
        return 1;
    wtRow *nt = new wtRow[n];
    if (!nt) {
        reportError("failed to get new tab\n");
        return 0;
    }
    for (int i = rangeStart; i < rangeEnd; i++) {
        nt[i] = rows[i];
        rows[i].clearEntries();
    }
    delete[] rows;
    rows = nt;
    rowCapacity = n;
    return 1;
}

bool workTable::hasEmptyRange()
{
    if (!rows)
        return 1;
    return rangeEnd <= rangeStart;
}

bool workTable::setRange()
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

bool workTable::setRange(int len)
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

bool workTable::setRange(int s, int e)
{
    if (!rows) {
        reportError("setRange - no table!\n");
        return 0;
    }
    rangeStart = s;
    rangeEnd = e;
    loopSize = 0;
    if (s < 0) {
        reportError("s before 0\n");
        abortAfterError();
        return 0;
    }
    if (e < s) {
        reportError("setRange : e<s before [%d,%d]\n", s, e);
        abortAfterError();
        return 0;
    }
    if (e >= rowCapacity) {
        reportError("e after end\n");
        abortAfterError();
        return 0;
    }
    return 1;
}

bool workTable::setSkip(int row, char skip)
{
    if (row < rangeStart) {
        reportError("setskip - before start\n");
        return 0;
    }
    if (row >= rangeEnd) {
        reportError("setskip >= end\n");
        return 0;
    }
    wtRow *targetRow = rowAt(row);
    if (!targetRow) {
        reportError("failed to get ie in setSkip\n");
        return 0;
    }
    targetRow->skip = skip;
    return 1;
}

int workTable::getRangeStart()
{
    if (!rows) {
        const char *m = codeModeStr(codeModeValue);
        const char *o = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("getStart called .. no table for %s mode %s\n", o, m);
        exitAfterCleanup(-1);
    }
    return rangeStart;
}

int workTable::getLoopSize()
{
    return loopSize;
}

int workTable::getRangeEnd()
{
    if (!rows) {
        const char *o = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("getEnd called .. no table for %s\n", o);
        reportError("getEnd called .. no table\n");
        exitAfterCleanup(-1);
    }
    return rangeEnd;
}

bool workTable::changeMode(unsigned char mode)
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

bool AddLinear_After(Block *blk, Instr *after, Instr *n);

bool workTable::logAddLinear_After(Instr *after, Instr *n)
{
    if (!n) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!n->setOwner((cls_unk30 *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!AddLinear_After((Block *)this, after, n)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, n)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool AddLinear_Head(Block *blk, Instr *n);

bool workTable::logAddLinear_Head(Instr *n)
{
    if (!n) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!n->setOwner((cls_unk30 *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!AddLinear_Head((Block *)this, n)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, n)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool AddLinear_Tail(Block *blk, Instr *n);

bool workTable::logAddLinear_Tail(Instr *n)
{
    if (!n) {
        reportError("can't add null pointer to linear\n");
        return 0;
    }
    if (!n->setOwner((cls_unk30 *)this)) {
        reportError("logAddLinear_Head: failed to set owner\n");
        return 0;
    }
    if (!AddLinear_Tail((Block *)this, n)) {
        reportError("logAddLinear_Head: failed to add head of linear list\n");
        return 0;
    }
    if (!logInstr((Block *)this, n)) {
        reportError("logAddLinear_Head: failed to log instruction\n");
        return 0;
    }
    return 1;
}

bool workTable::makeLinearList()
{
    clearLinear();
    if (rangeStart < 0 || !rows) {
        reportError("iTable::makeLinearList - table not ready\n");
        return 0;
    }
    for (int i = rangeStart; i < rangeEnd; i++) {
        Instr **pipelineEntries = ((cls_unk30 *)this)->getIE_ck(i);
        if (!pipelineEntries) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *instruction = pipelineEntries[p];
            if (instruction && !logAddLinear_Tail(instruction)) {
                reportError("iTable - failed to add an instruction\n");
                instruction->leDump();
                return 0;
            }
        }
    }
    return 1;
}

bool workTable::logUnlinkFromLinear(Instr *instruction)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!UnlinkFromLinear((Block *)this, instruction)) {
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

bool workTable::relinkRawIt()
{
    if (!rows) {
        reportError("relinkRawIt() -no table set\n");
        return 0;
    }
    clearLinear();
    for (int i = rangeStart; i < rangeEnd; i++) {
        wtRow *row = rowAt(i);
        if (!row) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *instruction = row->entries[p].instruction;
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

bool workTable::lockLine(InstructionPosition *line)
{
    return 1;
}

bool workTable::unlockLine(InstructionPosition *line)
{
    return 1;
}

workTable *workTable::localClone()
{
    if (codeModeValue != 1) {
        reportError("table to be locally cloned isn't uta\n");
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        reportError("failed to get new table\n");
        return 0;
    }
    nt->init(1, 0);
    nt->minimumSafeLoopCount = minimumSafeLoopCount;
    nt->statistics.copyCountersFrom(&statistics);
    nt->streamDependencyLength = streamDependencyLength;
    nt->streamLogicalLength = streamLogicalLength;
    nt->logicalInstructionLength = logicalInstructionLength;
    nt->externalMinimumLength = externalMinimumLength;
    unsigned int t = (unsigned char)allocationMode;
    nt->allocationMode = t;
    nt->nextInsertionSerial = 0;
    nt->ownerStream = ownerStream;
    regUseList &regs = registerUses;
    for (regUse *recordedUse = regs.head; recordedUse; recordedUse = recordedUse->next) {
        regUse *cloned = recordedUse->Clone();
        if (!nt->registerUses.append(cloned)) {
            reportError("failed to add cloned regref to list\n");
            delete recordedUse;
            return 0;
        }
    }
    for (Instr *instruction = linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        Instr *cloneInstruction = instruction->clone();
        for (int i = 0; i != cloneInstruction->operandCount; i++) {
            Operand *operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
            int mask = operand->componentMask & ~operand->externalPartMask;
            for (int k = 0; k != reg->partCount; k++) {
                int bit = 1 << k;
                if (mask & bit) {
                ol_s *operandLocation = operand->parts[k].operandLocations.head;
                if (!operandLocation || operandLocation->next) {
                    reportError("more or less than one oploc for non-extern input\n");
                    return 0;
                }
                Instr *setter = operandLocation->instruction;
                if (!setter) {
                    reportError("could not get setting op for setter\n");
                    return 0;
                }
                instStream *stream = setter->getOwningStream();
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
                if (!link_oParts(writerInstruction, p0, p1, cloneInstruction, i, k)) {
                    reportError("failed to link local oparts\n");
                    return 0;
                }
                }
            }
            }
        }
        if (!nt->logAddLinear_Tail(cloneInstruction)) {
            reportError("iTable::qInstruction - failed to add to linear list tail\n");
            return 0;
        }
    }
    nt->clearClones();
    clearClones();
    return nt;
}

bool workTable::replaceInstruction(Instr *old, Instr *nw)
{
    if (old->ownerTable != (cls_unk30 *)this) {
        reportError("old instruction isn't in table\n");
        return 0;
    }
    cls_unk30 *own = nw->ownerTable;
    if (own) {
        reportError("replaceInstruction - new instruction already has owner\n");
        return 0;
    }
    Instr *prev = old->linearLink.previousInstruction;
    if (!logUnlinkFromLinear(old)) {
        reportError("failed to take out old instruction from linear\n");
        return 0;
    }
    nw->linearOrderIndex = old->linearOrderIndex;
    nw->instructionSerial = old->instructionSerial;
    if (isAbstract()) {
        if (nw->isConcreteMove()) {
            reportError("trying to place a resolved move into abstract table\n");
            return 0;
        }
        if (!logAddLinear_After(prev, nw)) {
            reportError("failed to log new instruction \n");
            return 0;
        }
    } else {
        if (nw->isAbstractMove()) {
            reportError("trying to replace an abstract move into a table\n");
            return 0;
        }
        if (!logAddLinear_After(prev, nw)) {
            reportError("failed to log new instruction \n");
            return 0;
        }
        InstructionPosition pos;
        InstructionPosition *op = &old->position;
        pos.linearRow = op->linearRow;
        pos.loopRow = op->loopRow;
        pos.loopIteration = op->loopIteration;
        pos.pipelineIndex = op->pipelineIndex;
        InstructionPosition *position = &nw->position;
        position->linearRow = old->position.linearRow;
        position->loopRow = pos.loopRow;
        position->loopIteration = pos.loopIteration;
        position->pipelineIndex = pos.pipelineIndex;
        if (!takeOut(old)) {
            reportError("failed to take out old instruction\n");
            return 0;
        }
        wtRow *row = &rows[position->loopRow];
        row->entries[position->pipelineIndex].instruction = nw;
    }
    return 1;
}

bool workTable::destroyInstruction(Instr *&in)
{
    if (!logUnlinkFromLinear(in)) {
        reportError("destroyInstruction -  failed to unlink\n");
        return 0;
    }
    if (rows) {
        InstructionPosition *position = &in->position;
        int row = position->loopRow;
        int pipe = position->pipelineIndex;
        Instr **pipelineEntries = ((cls_unk30 *)this)->getIE_ck(row);
        if (!pipelineEntries) {
            reportError("logUnlinkFromLinear: failed to get IE where insn ought to be (line %d.%d)\n", row, pipe);
            return 0;
        }
        if (pipelineEntries[pipe] != in) {
            reportError("logUnlinkFromLinear: instruction wasn't where I thought it'd be!\n");
            return 0;
        }
        pipelineEntries[pipe] = 0;
    }
    in = 0;
    return 1;
}
