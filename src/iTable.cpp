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
    Instr *p = pendingInstructions.head;
    while (p) {
        Instr *next = p->pendingLink.nextInstruction;
        if (p->position.loopRow < 0)
            delete p;
        p = next;
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
        for (Instr *p = linearInstructions.head; p; p = p->linearLink.nextInstruction)
            p->dumpOperandDiagnostics(false);
        break;
    case 3:
    case 5:
        for (int i = rangeStart; i < rangeEnd; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                abortAfterError();
            r->refreshInstructionOperands(i);
        }
        break;
    case 2: {
        int lsz = loopSize;
        if (!rows) {
            reportError("code mode mid - but no table!\n");
            abortAfterError();
        }
        if (!lsz && rangeEnd < 0) {
            wtRow *r = rows;
            for (int n = rowCapacity - 1; n >= 0; n--, r++)
                if (r->entries[0].instruction || r->entries[1].instruction)
                    break;
        }
        for (int i = rangeStart; i < rangeEnd; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                abortAfterError();
            r->refreshInstructionOperands(i);
        }
        for (Instr *p = pendingInstructions.head; p; p = p->pendingLink.nextInstruction)
            p->dumpOperandDiagnostics(false);
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
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
    if (!ie) {
        reportError("failed to reserve instruction!\n");
        return false;
    }
    if (ie[slot]) {
        reportError("can't reserve instruction - something already there!\n");
        abortAfterError();
    }
    ie[slot] = (Instr *)key;
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
    Instr *b = getBranch();
    if (b && ((b->branchFlags >> 2) & 1))
        return b;
    return 0;
}

Instr *workTable::getBranch()
{
    Instr *b = 0;
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
            b = rowAt(r)->getEntry(1);
        }
        break;
    case 1:
        b = linearInstructions.tail;
        break;
    case 4:
        break;
    default: {
        const char *owner = ownerStream ? ownerStream->streamName() : "UNOWNED_TABLE";
        reportError("bad code mode for getBranch %s for %s\n", codeModeStr(codeModeValue), owner);
        abortAfterError();
    }
    }
    instStream *o = ownerStream;
    if (!o) {
        reportWarning("no owner set for getBranch\n");
        abortAfterError();
    } else if (b) {
        if (b->branchFlags) {
            if (b->tildePrefix)
                return 0;
            else if (!o->branchType) {
                const char *ownerName = o->streamName();
                reportError("iTable::getBranch() for %s table has a branch at the end, but owning stream doesn't think it branches\n",
                              ownerName);
                dump();
                abortAfterError();
            }
        } else if (o->branchType) {
            reportError("iTable::getBranch() - owner thinks it has a branch, but the instruction doesn't seem to be one\n");
            abortAfterError();
        }
    } else if (o->branchType) {
        reportError("iTable::getBranch - owning stream thinks it has a branch - but there's no instruction here!\n");
        abortAfterError();
    }
    return (b && b->branchFlags) ? b : 0;
}

bool UnlinkFromLinear(Block *blk, Instr *n);
bool decrementInstructionPipelineCounts(Block *blk, Instr *in);

inline bool workTable::unlinkAndUnlog(Instr *in)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!UnlinkFromLinear((Block *)this, in)) {
        reportError("logUnlinkFromLinear: failed to unlink instruction from linear\n");
        abortAfterError();
    }
    in->ownerTable = 0;
    if (!decrementInstructionPipelineCounts((Block *)this, in)) {
        reportError("failed to unlog instruction\n");
        return 0;
    }
    return 1;
}

bool workTable::removeBranch()
{
    Instr *b = getBranch();
    if (!b) {
        reportError("removeBranch - couldn't find a branch instruction\n");
        return 0;
    }
    if (!unlinkAndUnlog(b)) {
        reportError("failed to take out the branch instruction\n");
        return 0;
    }
    delete b;
    return 1;
}

int workTable::getCondLid()
{
    int lid = -1;
    Instr *b = getBranch();
    Instr *c = (b && ((b->branchFlags >> 2) & 1)) ? b : 0;
    if (c)
        lid = c->position.loopIteration;
    if (loopSize) {
        if (lid < 0)
            reportError("getCondLid : no terminating conditional!\n");
    } else if (lid > 0) {
        reportError("getCondLid : terminating conditional had lid>0\n");
    }
    return lid;
}

bool workTable::takeOut(Instr *i)
{
    InstructionPosition *p = &i->position;
    wtRow *row = &rows[p->loopRow];
    row->entries[p->pipelineIndex].instruction = (Instr *)i->replacedInstructionWord;
    i->clearSlot();
    return 1;
}

bool workTable::putIn(Instr *i)
{
    InstructionPosition *p = &i->position;
    wtRow *row = &rows[p->loopRow];
    row->entries[p->pipelineIndex].instruction = i;
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
        wtRow *sr = &from->rows[i];
        wtRow *dr = &to->rows[j];
        for (int p = 0; p != 2; p++) {
            Instr *c;
            Instr *in = sr->getEntry(p);
            c = 0;
            if (dr->getEntry(p)) {
                reportError("CloneAppend: would overwrite inst in target\n");
                return 0;
            }
            if (in) {
                if (!in->opcodeInfo) {
                    reportError("no II in inst\n");
                    from->dump();
                }
                if (!in->isNop() || a7) {
                    int lid = in->position.loopIteration;
                    if (lid <= lidS && lid >= lidE) {
                        unsigned char f = in->branchFlags;
                        if ((f && a5) || !in->getBranchType()) {
                            c = in->clone();
                            if (!c) {
                                reportError("failed to clone instruction\n");
                                return 0;
                            }
                            c->cloneImage = 0;
                            in->cloneImage = 0;
                            if (a6)
                                c->expectedAliasLoopId = lidS - lid;
                        }
                    }
                }
            }
            dr->entries[p].instruction = c;
            if (c) {
                InstructionPosition pos;
                c->setOwner((cls_unk30 *)to);
                pos.setRow(j);
                pos.pipelineIndex = p;
                InstructionPosition *loc = &c->position;
                c->position.linearRow = pos.linearRow;
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
    for (Instr *i = linearInstructions.head; i; i = i->linearLink.nextInstruction)
        if (i->syntheticKind)
            i->dumpOperandDiagnostics(false);
}

bool workTable::clearClones()
{
    for (Instr *i = linearInstructions.head; i; i = i->linearLink.nextInstruction)
        i->cloneImage = 0;
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

void trackEarliestInsertion(Instr *in)
{
    if (in) {
        if (!earliestInsertionInstruction || in->insertionSerial < earliestInsertionInstruction->insertionSerial)
            earliestInsertionInstruction = in;
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
    wtRow *r = rowAt(rangeEnd - 1);
    if (!r)
        return 0;
    Instr *in = r->entries[1].instruction;
    if (in) {
        if (in->ramAccessType || in->branchFlags)
            return 0;
        int op = getOpcodeId(in->opcodeInfo);
        if (op >= 165 && op <= 167)
            return 0;
    }
    r = rowAt(rangeEnd - 2);
    if (!r)
        return 1;
    in = r->entries[1].instruction;
    if (in && getOpcodeId(in->opcodeInfo) == 165)
        return 0;
    return 1;
}

Instr **cls_unk30::getIE_ck(int rl)
{
    workTable *t = (workTable *)this;
    if (!t->rows) {
        reportError("checkie: table not set\n");
        return 0;
    }
    if (rl < 0) {
        reportError("checkie() - rl<0\n");
        return 0;
    }
    if (t->loopSize) {
        if (t->rangeStart) {
            reportError("getIE_ck - iStart not zero for loop table\n");
            abortAfterError();
        }
        if (rl >= t->loopSize)
            rl %= t->loopSize;
    }
    if (rl >= t->rowCapacity) {
        reportError("checkie() - rl>=m_MaxSize %d,%d\n", rl, t->rowCapacity);
        return 0;
    }
    return (Instr **)&t->rows[rl];
}

Instr *workTable::getInstr(int rl, int p)
{
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
    if (!ie)
        return 0;
    if (p < 0 || p >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return ie[p];
}

static inline Instr *getPipeInstr(Instr **ie, int p)
{
    if (!ie)
        return 0;
    if (p < 0 || p >= 2) {
        reportError("out of bounds pipe\n");
        return 0;
    }
    return ie[p];
}

Instr *workTable::getInstrInRange(int rl, int p)
{
    if (rl < rangeStart || rl >= rangeEnd)
        return 0;
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
    return getPipeInstr(ie, p);
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

bool logInstr(Block *blk, Instr *in)
{
    if (in->permittedPipelineMask == 3)
        blk->dualPipelineInstructionCount++;
    else {
        int n = 0;
        if (in->permittedPipelineMask & 1) {
            blk->instructionCountsByPipe[0]++;
            n = 1;
        }
        if (in->permittedPipelineMask & 2) {
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

bool decrementInstructionPipelineCounts(Block *blk, Instr *in)
{
    if (in->permittedPipelineMask == 3)
        blk->dualPipelineInstructionCount--;
    else {
        int n = 0;
        if (in->permittedPipelineMask & 1) {
            blk->instructionCountsByPipe[0]--;
            n = 1;
        }
        if (in->permittedPipelineMask & 2) {
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
    for (Instr *i = completedInstructions.head; i; i = i->pendingLink.nextInstruction) {
        int row = i->position.linearRow;
        if (row < 0) {
            reportError("getCurrentDoneSize()\n");
            return -1;
        }
        short m = i->externalOutputConstraint;
        if (m < i->externalOutputPairedConstraint)
            m = i->externalOutputPairedConstraint;
        int minToEnd = m;
        if (minToEnd < 0) {
            reportError("getCurrentDoneSize() - minToEnd was bad\n");
            return -1;
        }
        if (i->branchFlags && minToEnd <= 0)
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
    Instr *i = completedInstructions.head;
    *maxLid = -1;
    *maxBranchLid = -1;
    *minSfx1 = 30000;
    *minSfx2 = 30000;
    for (; i; i = i->pendingLink.nextInstruction) {
        int lid = i->position.loopIteration;
        if (lid > *maxLid)
            *maxLid = lid;
        switch ((unsigned char)i->schedulerCategory) {
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
        if (i->branchFlags && lid > *maxBranchLid)
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
    wtRow *r = rowAt(row);
    if (!r) {
        reportError("failed to get ie in setSkip\n");
        return 0;
    }
    r->skip = skip;
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
        Instr **ie = ((cls_unk30 *)this)->getIE_ck(i);
        if (!ie) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *in = ie[p];
            if (in && !logAddLinear_Tail(in)) {
                reportError("iTable - failed to add an instruction\n");
                in->leDump();
                return 0;
            }
        }
    }
    return 1;
}

bool workTable::logUnlinkFromLinear(Instr *in)
{
    if (pendingInstructions.count) {
        reportError("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", pendingInstructions.count);
        return 0;
    }
    if (!UnlinkFromLinear((Block *)this, in)) {
        reportError("logUnlinkFromLinear: failed to unlink instruction from linear\n");
        abortAfterError();
    }
    in->ownerTable = 0;
    if (!decrementInstructionPipelineCounts((Block *)this, in)) {
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
        wtRow *r = rowAt(i);
        if (!r) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *in = r->entries[p].instruction;
            if (in) {
                in->linearLink.previousInstruction = 0;
                in->linearLink.nextInstruction = 0;
                if (!logAddLinear_Tail(in)) {
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
    for (regUse *r = regs.head; r; r = r->next) {
        regUse *cloned = r->Clone();
        if (!nt->registerUses.append(cloned)) {
            reportError("failed to add cloned regref to list\n");
            delete r;
            return 0;
        }
    }
    for (Instr *in = linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        Instr *c = in->clone();
        for (int i = 0; i != c->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
            int mask = op->componentMask & ~op->externalPartMask;
            for (int k = 0; k != reg->partCount; k++) {
                int bit = 1 << k;
                if (mask & bit) {
                ol_s *o = op->parts[k].operandLocations.head;
                if (!o || o->next) {
                    reportError("more or less than one oploc for non-extern input\n");
                    return 0;
                }
                Instr *setter = o->instruction;
                if (!setter) {
                    reportError("could not get setting op for setter\n");
                    return 0;
                }
                instStream *s = setter->getOwningStream();
                if (s != ownerStream) {
                    reportError("owner of setter not us %p vs %p\n", setter->ownerTable, this, s);
                    return 0;
                }
                if (!setter->cloneImage) {
                    reportError("qInstruction - setter had null work clone set\n");
                    return 0;
                }
                Instr *w = setter->cloneImage;
                int p1 = o->componentIndex;
                int p0 = o->operandIndex;
                if (!link_oParts(w, p0, p1, c, i, k)) {
                    reportError("failed to link local oparts\n");
                    return 0;
                }
                }
            }
            }
        }
        if (!nt->logAddLinear_Tail(c)) {
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
        InstructionPosition *p = &nw->position;
        p->linearRow = old->position.linearRow;
        p->loopRow = pos.loopRow;
        p->loopIteration = pos.loopIteration;
        p->pipelineIndex = pos.pipelineIndex;
        if (!takeOut(old)) {
            reportError("failed to take out old instruction\n");
            return 0;
        }
        wtRow *row = &rows[p->loopRow];
        row->entries[p->pipelineIndex].instruction = nw;
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
        InstructionPosition *p = &in->position;
        int row = p->loopRow;
        int pipe = p->pipelineIndex;
        Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
        if (!ie) {
            reportError("logUnlinkFromLinear: failed to get IE where insn ought to be (line %d.%d)\n", row, pipe);
            return 0;
        }
        if (ie[pipe] != in) {
            reportError("logUnlinkFromLinear: instruction wasn't where I thought it'd be!\n");
            return 0;
        }
        ie[pipe] = 0;
    }
    in = 0;
    return 1;
}
