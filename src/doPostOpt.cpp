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


static inline void dumpOptInstruction(Instr *in)
{
    in->dumpOperandDiagnostics(false);
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
        unk_0b = 1;
        if (pendingInstructions.count) {
            reportError("doPostOpt - but there's things still to do\n");
            return false;
        }
        instStream *owner = ownerStream;
        if (!owner) {
            reportError("doPostOpt - not get owner\n");
            return false;
        }
        workTable *pw = owner->getTable();
        if (!pw) {
            reportError("failed to get work table from owner\n");
            return false;
        }
        if (pw == this) {
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
            assignLength(length, rawLengthBits(pw->func_08072eec()));
            break;
        }
        if (!setTableRawMode(rawMode)) {
            reportError("doPostOpt : failed to set raw mode..\n");
            return false;
        }
        int rl;
        for (rl = rangeStart; rl < rangeEnd; rl++) {
            Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
            if (!ie) {
                reportError("iTable::doPostOpt - failed to get ientry\n");
                return false;
            }
            if (((wtRow *)ie)->skip) {
                reportError("bSkip is set\n");
                return false;
            }
            for (int p = 0; p != 2; p++) {
                Instr *in = ie[p];
                if (in) {
                    int nBranch;
                    switch (rawMode) {
                    case 2: {
                        int t = in->position.unk_04;
                        in->scheduledCycle = t;
                        if (rangeStart) {
                            reportError("postopt - istart set for fixed mode \n");
                            return false;
                        }
                        break;
                    }
                    case 1: {
                        nBranch = -1;
                        int t = in->position.unk_04;
                        in->scheduledCycle = t - rangeStart;
                        break;
                    }
                    case 3:
                    case 4: {
                        Instr *c = in->unk_08;
                        if (!c) {
                            reportError("no work clone set\n");
                            return false;
                        }
                        short t = c->scheduledCycle;
                        if (t < 0 || t >= *(int *)&length) {
                            reportError("emutick bad for raw postopt\n");
                            return false;
                        }
                        in->scheduledCycle = t;
                        break;
                    }
                    default:
                        reportError("raw post mode for postopt\n");
                        return false;
                    }
                    if (!in->unk_1428) {
                        if (in->unk_30 != (cls_unk30 *)this) {
                            reportError("owner not this for non-move...%p\n", in->unk_30);
                            dumpOptInstruction(in);
                            return false;
                        }
                        if (!logUnlinkFromLinear(in)) {
                            reportError("doPostOpt:failed to unlink from linear list\n");
                            return false;
                        }
                        count++;
                    } else {
                        if (in->unk_30) {
                            reportError("owner not 0 for move\n");
                            return false;
                        }
                    }
                    if (!UnlinkFromDone((Block *)this, in)) {
                        reportError("doPostOpt -  unlink from done list failed\n");
                        return false;
                    }
                    if (in->branchFlags) {
                        if (nBranch + 1) {
                            reportError("doPostOpt - more than one branch\n");
                            return false;
                        }
                        if (rl != rangeEnd - 2) {
                            reportError("badly placed branch detected in postopt\n");
                            dump();
                            return false;
                        }
                    }
                    InstructionPosition *loc = &in->position;
                    int lid = loc->unk_03;
                    int row = loc->unk_04;
                    int iloc = in->position.unk_00;
                    if (!loopSize && lid) {
                        reportError("i_lid set not in loop\n");
                        return false;
                    }
                    int rebuilt = lid * g_080b336c;
                    rebuilt += row;
                    if (iloc != rebuilt) {
                        reportError("badly formed iloc in instruction\n");
                        return false;
                    }
                    if (row != rl) {
                        reportError("instruction not where it thinks it is\n");
                        return false;
                    }
                    if (lid > maxLid)
                        maxLid = lid;
                    if (!ie[p]) {
                        reportError("damn - lost instruction at %d:%d!\n", rl, p);
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
        for (rl = rangeStart; rl < rangeEnd; rl++) {
            Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
            if (!ie)
                reportError("can't get ie during postopt\n");
            for (int p = 0; p != 2; p++) {
                Instr *in = ie[p];
                if (in) {
                    if (!logAddLinear_Tail(in)) {
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
        unk_0c = *(int *)&length;
    }
    return true;
}

static inline bool isRawTable(workTable *wt)
{
    return wt->codeModeValue == 3;
}

bool instStream::setOptTable(workTable *nt)
{
    if (!nt) {
        reportError("can't set opt table to null ptr - use clear\n");
        abortAfterError();
    }
    if (!isRawTable(nt)) {
        reportError("setOptTable - table not vsm\n");
        return false;
    }
    workTable *old = getTable();
    if (!old) {
        reportError("no old work table\n");
        return false;
    }
    if (old == nt) {
        reportError("old and new are same\n");
        return false;
    }
    if (optimizedTable) {
        reportError("stream already has opttable set\n");
        return false;
    }
    for (Instr *in = nt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        if (in->getOwningStream() != this) {
            reportError("didn't own instruction\n");
            return false;
        }
        if (in->unk_00) {
            reportError("doPostLinearOpt - optClone set in image?\n");
            return false;
        }
        Instr *c = in->unk_08;
        if (!c) {
            if (!in->unk_1428) {
                reportError("doPostLinearOpt - inst wasn't cloned, but not a move either\n");
                dumpOptInstruction(in);
                return false;
            }
        } else {
            if (c->unk_00) {
                reportError("doPostLinear() - original had m_optClone set\n");
                dumpOptInstruction(c);
                return false;
            }
            if (c->getOwningStream() != this) {
                reportError("setOptTable -  cloned image wsn't owned by us\n");
                return false;
            }
            workTable *owner = (workTable *)c->unk_30;
            if (owner != getTable()) {
                reportError("clone owner wasn't our work table\n");
                return false;
            }
            if (in->operandCount != c->operandCount) {
                reportError("operand count mismatch between original and opt image\n");
                return false;
            }
            for (int i = 0; i != in->operandCount; i++) {
                char io = in->operands[i].direction;
                char cio = c->operands[i].direction;
                if (io != cio) {
                    if (cio != 1 && io != 3) {
                        reportError("io type mismatch for something that wasn't a whacked input\n");
                        return false;
                    }
                    in->operands[i].assignedRegister = c->operands[i].assignedRegister;
                    in->operands[i].direction = cio;
                }
            }
            in->unk_04 = (VclPtrWord)c;
            c->unk_00 = (VclPtrWord)in;
        }
    }
    nt->clearClones();
    old->clearClones();
    workTablePtr = nt;
    optimizedTable = nt;
    if (nt->unk_00 >= 0)
        unk_144 |= 1 << nt->unk_00;
    if (nt->getLoopSize())
        unk_140 = 1;
    return true;
}
