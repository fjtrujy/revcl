#include "os.h"
// iTable: work table construction and management (CIE group 24).
#include "workTable.h"
#include "instStream.h"
#include "Block.h"

workTable::workTable()
{
    unk_2c = 0;
    unk_4c = 0;
    unk_24 = 0;
    unk_20 = 0;
    unk_0b = 0;
    unk_10 = 0;
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
        unk_2c = new wtRow[size];
        if (!unk_2c)
            return false;
        unk_28 = size;
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
    unk_30 = 0;
    unk_34 = 0;
    unk_38 = 0;
    unk_ac = 0;
    unk_40 = 0;
    unk_ad = 0;
    unk_0c = 0;
    unk_7a = 0;
    unk_09 = mode;
    return true;
}

bool workTable::dispose()
{
    Instr *p = pendingInstructions.head;
    while (p) {
        Instr *next = p->unk_24.unk_00;
        if (p->position.unk_04 < 0)
            delete p;
        p = next;
    }
    unk_0b = 0;
    if (!freeLockLists(unk_24)) {
        reportError("failed to dispose rll list\n");
        abortAfterError();
    }
    unk_24 = 0;
    unk_20 = 0;
    unk_14.clear();
    delete[] unk_2c;
    unk_2c = 0;
    unk_28 = 0;
    unk_38 = 0;
    unk_30 = 0;
    unk_34 = 0;
    unk_3c = 0;
    unk_00 = -1;
    unk_09 = 0;
    unk_08 = 0;
    unk_0a = 0;
    instructionCountsByPipe[2] = 0;
    unk_bc = 0;
    vectorMoveCount = 0;
    oddPipeMoveCount = 0;
    instructionCountsByPipe[0] = 0;
    evenPipeMoveCount = 0;
    instructionCountsByPipe[1] = 0;
    unk_4c = 0;
    linearInstructions.reset();
    pendingInstructions.reset();
    completedInstructions.reset();
    unk_50 = 0;
    unk_40 = 0;
    unk_0c = -1;
    unk_10 = 0;
    return true;
}

void workTable::dump()
{
    switch (unk_09) {
    case 0:
        break;
    case 1:
    case 4:
        for (Instr *p = linearInstructions.head; p; p = p->unk_1c.unk_00)
            p->func_08054008(false);
        break;
    case 3:
    case 5:
        for (int i = unk_30; i < unk_34; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                abortAfterError();
            r->refreshInstructionOperands(i);
        }
        break;
    case 2: {
        int lsz = unk_38;
        if (!unk_2c) {
            reportError("code mode mid - but no table!\n");
            abortAfterError();
        }
        if (!lsz && unk_34 < 0) {
            wtRow *r = unk_2c;
            for (int n = unk_28 - 1; n >= 0; n--, r++)
                if (r->entries[0].unk_00 || r->entries[1].unk_00)
                    break;
        }
        for (int i = unk_30; i < unk_34; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                abortAfterError();
            r->refreshInstructionOperands(i);
        }
        for (Instr *p = pendingInstructions.head; p; p = p->unk_24.unk_00)
            p->func_08054008(false);
        break;
    }
    default:
        reportError("unknown code mode for Dump()\n");
        abortAfterError();
    }
    unk_14.dumpRegisterUses();
}

const char *workTable::ownerName()
{
    if (!unk_10)
        return "UNOWNED_TABLE";
    return unk_10->streamName();
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

int workTable::func_08072eec()
{
    int len = -1;
    switch (unk_09) {
    case 3:
        len = unk_0c;
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
    switch (unk_09) {
    case 1:
    case 3:
        len = unk_78;
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
    switch (unk_09) {
    case 3:
        if (!unk_2c) {
            reportError("no table!\n");
            abortAfterError();
        }
        if (unk_34 < unk_30) {
            reportError("end before start\n");
            abortAfterError();
        }
        len = unk_34 - unk_30;
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
    switch (unk_09) {
    case 3:
        if (!unk_2c) {
            reportWarning("called getBranch - but no table\n");
            return 0;
        }
        if (unk_34 > unk_30) {
            int r = unk_34 - 2;
            if (r < unk_30)
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
        const char *owner = unk_10 ? unk_10->streamName() : "UNOWNED_TABLE";
        reportError("bad code mode for getBranch %s for %s\n", codeModeStr(unk_09), owner);
        abortAfterError();
    }
    }
    instStream *o = unk_10;
    if (!o) {
        reportWarning("no owner set for getBranch\n");
        abortAfterError();
    } else if (b) {
        if (b->branchFlags) {
            if (b->unk_1392)
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
bool func_08073bb0(Block *blk, Instr *in);

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
    in->unk_30 = 0;
    if (!func_08073bb0((Block *)this, in)) {
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
        lid = c->position.unk_03;
    if (unk_38) {
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
    wtRow *row = &unk_2c[p->unk_04];
    row->entries[p->unk_02].unk_00 = (Instr *)i->unk_1398;
    i->clearSlot();
    return 1;
}

bool workTable::putIn(Instr *i)
{
    InstructionPosition *p = &i->position;
    wtRow *row = &unk_2c[p->unk_04];
    row->entries[p->unk_02].unk_00 = i;
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
    if (!to->unk_2c) {
        reportError("clone append - source table has no attached table\n");
        return 0;
    }
    if (!from->unk_2c) {
        reportError("clone append - destination table has no attached table\n");
        return 0;
    }
    int n = from->unk_34 - from->unk_30;
    int need = n + to->unk_34;
    if (need > to->unk_28 && !to->grow(need + 0x20)) {
        reportError("uh-oh, failed to grow a table\n");
        return 0;
    }
    int j, i;
    for (j = to->unk_34, i = from->unk_30; i != from->unk_34; i++, j++) {
        wtRow *sr = &from->unk_2c[i];
        wtRow *dr = &to->unk_2c[j];
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
                    int lid = in->position.unk_03;
                    if (lid <= lidS && lid >= lidE) {
                        unsigned char f = in->branchFlags;
                        if ((f && a5) || !in->getBranchType()) {
                            c = in->clone();
                            if (!c) {
                                reportError("failed to clone instruction\n");
                                return 0;
                            }
                            c->unk_08 = 0;
                            in->unk_08 = 0;
                            if (a6)
                                c->unk_13d2 = lidS - lid;
                        }
                    }
                }
            }
            dr->entries[p].unk_00 = c;
            if (c) {
                InstructionPosition pos;
                c->setOwner((cls_unk30 *)to);
                pos.setRow(j);
                pos.unk_02 = p;
                InstructionPosition *loc = &c->position;
                c->position.unk_00 = pos.unk_00;
                loc->unk_04 = pos.unk_04;
                loc->unk_03 = pos.unk_03;
                loc->unk_02 = pos.unk_02;
            }
        }
    }
    to->unk_34 += n;
    return 1;
}

void workTable::dumpSynthetics()
{
    for (Instr *i = linearInstructions.head; i; i = i->unk_1c.unk_00)
        if (i->syntheticKind)
            i->func_08054008(false);
}

bool workTable::clearClones()
{
    for (Instr *i = linearInstructions.head; i; i = i->unk_1c.unk_00)
        i->unk_08 = 0;
    return 1;
}

int workTable::getTableRawMode()
{
    if (unk_09 == 3 || unk_09 == 5)
        return unk_0a;
    return 0;
}

bool workTable::setTableRawMode(unsigned char mode)
{
    if (unk_09 != 3 && unk_09 != 5 && mode) {
        int cm = unk_09;
        reportError("failed to set vsm mode - because code mode isn't vsm..mode=%d cm=%d\n", mode, cm);
        abortAfterError();
        return 0;
    }
    unk_0a = mode;
    return 1;
}

Instr *g_080de72c;

void func_080737b8(Instr *in)
{
    if (in) {
        if (!g_080de72c || in->unk_1396 < g_080de72c->unk_1396)
            g_080de72c = in;
    }
}

bool workTable::resetFlowStalls()
{
    unk_0c = -1;
    if (unk_2c && unk_30 >= 0) {
        for (int i = unk_30; i < unk_34; i++)
            if (!unk_2c[i].clearStalls()) {
                reportError("failed to flow reset ientry\n");
                return 0;
            }
    }
    return 1;
}

bool workTable::func_08073854()
{
    if (unk_30 < 0)
        return 0;
    if (unk_34 <= 0)
        return 1;
    wtRow *r = rowAt(unk_34 - 1);
    if (!r)
        return 0;
    Instr *in = r->entries[1].unk_00;
    if (in) {
        if (in->ramAccessType || in->branchFlags)
            return 0;
        int op = getOpcodeId(in->opcodeInfo);
        if (op >= 165 && op <= 167)
            return 0;
    }
    r = rowAt(unk_34 - 2);
    if (!r)
        return 1;
    in = r->entries[1].unk_00;
    if (in && getOpcodeId(in->opcodeInfo) == 165)
        return 0;
    return 1;
}

Instr **cls_unk30::getIE_ck(int rl)
{
    workTable *t = (workTable *)this;
    if (!t->unk_2c) {
        reportError("checkie: table not set\n");
        return 0;
    }
    if (rl < 0) {
        reportError("checkie() - rl<0\n");
        return 0;
    }
    if (t->unk_38) {
        if (t->unk_30) {
            reportError("getIE_ck - iStart not zero for loop table\n");
            abortAfterError();
        }
        if (rl >= t->unk_38)
            rl %= t->unk_38;
    }
    if (rl >= t->unk_28) {
        reportError("checkie() - rl>=m_MaxSize %d,%d\n", rl, t->unk_28);
        return 0;
    }
    return (Instr **)&t->unk_2c[rl];
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
    if (rl < unk_30 || rl >= unk_34)
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
    if (in->unk_138c == 3)
        blk->unk_bc++;
    else {
        int n = 0;
        if (in->unk_138c & 1) {
            blk->instructionCountsByPipe[0]++;
            n = 1;
        }
        if (in->unk_138c & 2) {
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

bool func_08073bb0(Block *blk, Instr *in)
{
    if (in->unk_138c == 3)
        blk->unk_bc--;
    else {
        int n = 0;
        if (in->unk_138c & 1) {
            blk->instructionCountsByPipe[0]--;
            n = 1;
        }
        if (in->unk_138c & 2) {
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
    unk_bc = 0;
    vectorMoveCount = 0;
    oddPipeMoveCount = 0;
    instructionCountsByPipe[0] = 0;
    evenPipeMoveCount = 0;
    instructionCountsByPipe[1] = 0;
}

int workTable::getCurrentDoneSize()
{
    if (unk_38)
        return unk_38;
    int size = 0;
    for (Instr *i = completedInstructions.head; i; i = i->unk_24.unk_00) {
        int row = i->position.unk_00;
        if (row < 0) {
            reportError("getCurrentDoneSize()\n");
            return -1;
        }
        short m = i->unk_13b6;
        if (m < i->unk_13b4)
            m = i->unk_13b4;
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
    if (size < unk_44)
        size = unk_44;
    return size;
}

bool workTable::getCurrentLidStats(int *maxLid, int *maxBranchLid, int *minSfx1, int *minSfx2)
{
    Instr *i = completedInstructions.head;
    *maxLid = -1;
    *maxBranchLid = -1;
    *minSfx1 = 30000;
    *minSfx2 = 30000;
    for (; i; i = i->unk_24.unk_00) {
        int lid = i->position.unk_03;
        if (lid > *maxLid)
            *maxLid = lid;
        switch ((unsigned char)i->unk_138e) {
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
    if (!unk_2c) {
        reportError("can't grow.. there's nothing here\n");
        return 0;
    }
    if (n <= unk_28)
        return 1;
    wtRow *nt = new wtRow[n];
    if (!nt) {
        reportError("failed to get new tab\n");
        return 0;
    }
    for (int i = unk_30; i < unk_34; i++) {
        nt[i] = unk_2c[i];
        unk_2c[i].clearEntries();
    }
    delete[] unk_2c;
    unk_2c = nt;
    unk_28 = n;
    return 1;
}

bool workTable::hasEmptyRange()
{
    if (!unk_2c)
        return 1;
    return unk_34 <= unk_30;
}

bool workTable::setRange()
{
    if (!unk_2c) {
        reportError("setRange - no table!\n");
        return 0;
    }
    unk_30 = 0;
    unk_34 = -1;
    unk_38 = 0;
    return 1;
}

bool workTable::setRange(int len)
{
    if (!unk_2c) {
        reportError("setRange - no table!\n");
        return 0;
    }
    unk_30 = 0;
    unk_34 = len;
    unk_38 = len;
    return 1;
}

bool workTable::setRange(int s, int e)
{
    if (!unk_2c) {
        reportError("setRange - no table!\n");
        return 0;
    }
    unk_30 = s;
    unk_34 = e;
    unk_38 = 0;
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
    if (e >= unk_28) {
        reportError("e after end\n");
        abortAfterError();
        return 0;
    }
    return 1;
}

bool workTable::setSkip(int row, char skip)
{
    if (row < unk_30) {
        reportError("setskip - before start\n");
        return 0;
    }
    if (row >= unk_34) {
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
    if (!unk_2c) {
        const char *m = codeModeStr(unk_09);
        const char *o = unk_10 ? unk_10->streamName() : "UNOWNED_TABLE";
        reportError("getStart called .. no table for %s mode %s\n", o, m);
        exitAfterCleanup(-1);
    }
    return unk_30;
}

int workTable::getLoopSize()
{
    return unk_38;
}

int workTable::getRangeEnd()
{
    if (!unk_2c) {
        const char *o = unk_10 ? unk_10->streamName() : "UNOWNED_TABLE";
        reportError("getEnd called .. no table for %s\n", o);
        reportError("getEnd called .. no table\n");
        exitAfterCleanup(-1);
    }
    return unk_34;
}

bool workTable::changeMode(unsigned char mode)
{
    int cur = unk_09;
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
    unk_09 = mode;
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
    if (unk_30 < 0 || !unk_2c) {
        reportError("iTable::makeLinearList - table not ready\n");
        return 0;
    }
    for (int i = unk_30; i < unk_34; i++) {
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
    in->unk_30 = 0;
    if (!func_08073bb0((Block *)this, in)) {
        reportError("failed to unlog instruction\n");
        return 0;
    }
    return 1;
}

bool workTable::relinkRawIt()
{
    if (!unk_2c) {
        reportError("relinkRawIt() -no table set\n");
        return 0;
    }
    clearLinear();
    for (int i = unk_30; i < unk_34; i++) {
        wtRow *r = rowAt(i);
        if (!r) {
            reportError("iTable::makeLinearList - failed to get ientry\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *in = r->entries[p].unk_00;
            if (in) {
                in->unk_1c.unk_04 = 0;
                in->unk_1c.unk_00 = 0;
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
    if (unk_09 != 1) {
        reportError("table to be locally cloned isn't uta\n");
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        reportError("failed to get new table\n");
        return 0;
    }
    nt->init(1, 0);
    nt->unk_7a = unk_7a;
    nt->unk_5c.copyCountersFrom(&unk_5c);
    nt->unk_74 = unk_74;
    nt->unk_76 = unk_76;
    nt->unk_78 = unk_78;
    nt->unk_44 = unk_44;
    unsigned int t = (unsigned char)unk_08;
    nt->unk_08 = t;
    nt->unk_40 = 0;
    nt->unk_10 = unk_10;
    regUseList &regs = unk_14;
    for (regUse *r = regs.head; r; r = r->unk_04) {
        regUse *cloned = r->Clone();
        if (!nt->unk_14.append(cloned)) {
            reportError("failed to add cloned regref to list\n");
            delete r;
            return 0;
        }
    }
    for (Instr *in = linearInstructions.head; in; in = in->unk_1c.unk_00) {
        Instr *c = in->clone();
        for (int i = 0; i != c->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
            int mask = op->unk_07 & ~op->unk_27;
            for (int k = 0; k != reg->partCount; k++) {
                int bit = 1 << k;
                if (mask & bit) {
                ol_s *o = op->unk_2c[k].unk_54.head;
                if (!o || o->unk_00) {
                    reportError("more or less than one oploc for non-extern input\n");
                    return 0;
                }
                Instr *setter = o->unk_04;
                if (!setter) {
                    reportError("could not get setting op for setter\n");
                    return 0;
                }
                instStream *s = setter->getOwningStream();
                if (s != unk_10) {
                    reportError("owner of setter not us %p vs %p\n", setter->unk_30, this, s);
                    return 0;
                }
                if (!setter->unk_08) {
                    reportError("qInstruction - setter had null work clone set\n");
                    return 0;
                }
                Instr *w = setter->unk_08;
                int p1 = o->unk_11;
                int p0 = o->unk_10;
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
    if (old->unk_30 != (cls_unk30 *)this) {
        reportError("old instruction isn't in table\n");
        return 0;
    }
    cls_unk30 *own = nw->unk_30;
    if (own) {
        reportError("replaceInstruction - new instruction already has owner\n");
        return 0;
    }
    Instr *prev = old->unk_1c.unk_04;
    if (!logUnlinkFromLinear(old)) {
        reportError("failed to take out old instruction from linear\n");
        return 0;
    }
    nw->linearOrderIndex = old->linearOrderIndex;
    nw->unk_13e0 = old->unk_13e0;
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
        pos.unk_00 = op->unk_00;
        pos.unk_04 = op->unk_04;
        pos.unk_03 = op->unk_03;
        pos.unk_02 = op->unk_02;
        InstructionPosition *p = &nw->position;
        p->unk_00 = old->position.unk_00;
        p->unk_04 = pos.unk_04;
        p->unk_03 = pos.unk_03;
        p->unk_02 = pos.unk_02;
        if (!takeOut(old)) {
            reportError("failed to take out old instruction\n");
            return 0;
        }
        wtRow *row = &unk_2c[p->unk_04];
        row->entries[p->unk_02].unk_00 = nw;
    }
    return 1;
}

bool workTable::destroyInstruction(Instr *&in)
{
    if (!logUnlinkFromLinear(in)) {
        reportError("destroyInstruction -  failed to unlink\n");
        return 0;
    }
    if (unk_2c) {
        InstructionPosition *p = &in->position;
        int row = p->unk_04;
        int pipe = p->unk_02;
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
