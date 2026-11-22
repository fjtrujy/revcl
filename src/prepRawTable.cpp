#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRangeFrom(workTable *t);
void resetRange();

static inline void dumpRawInstruction(Instr *in)
{
    in->func_08054008(false);
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
    unk_50 = 0;
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
    for (int rl = 0; rl < rangeEnd; rl++) {
        wtRow *ie = (wtRow *)((cls_unk30 *)this)->getIE_ck(rl);
        if (!ie) {
            reportError("iTable::prepTable - failed to get ientry\n");
            return 0;
        }
        if (ie->skip) {
            reportError("bSkip set for preptabl\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *in = ie->entries[p].unk_00;
            if (!in)
                continue;
            if (in->syntheticKind) {
                reportError("synth instruction in preptab\n");
                in->leDump();
                abortAfterError();
            }
            if (in->parsedLoopId != -1) {
                reportError("parseLid isn't cleared in prepTab\n");
                in->func_08054008(false);
                exitAfterCleanup(-1);
            }
            int lid = 0;
            int il;
            InstructionPosition *loc;
            if (loopSize) {
                if (!in->isNop())
                    lid = in->position.unk_03;
                if (lid < 0) {
                    reportError("lid was <0 during table setup\n");
                    in->leDump();
                    return 0;
                }
                loc = &in->position;
                loc->unk_03 = lid;
                il = lid * loopSize + rl;
            } else {
                loc = &in->position;
                lid = loc->unk_03;
                if (lid) {
                    reportError("non zero lid for non-loop %d\n", lid);
                    dumpRawInstruction(in);
                    return 0;
                }
                loc->unk_03 = 0;
                il = rl;
            }
            if (lid > maxLid)
                maxLid = lid;
            InstructionPosition pos;
            pos.setFromLinearRow(il);
            pos.setInstructionPipe(p);
            in->position.unk_00 = pos.unk_00;
            signed char nl = pos.unk_03;
            loc->unk_04 = pos.unk_04;
            loc->unk_03 = nl;
            loc->unk_02 = pos.unk_02;
            if (nl != lid) {
                reportError("iTable::initialSetup - lid was bad (%d!=%d) lsz=%d rl=%d il=%d\n",
                              nl, lid, loopSize, il, rl);
                return 0;
            }
            if (pos.unk_04 != rl) {
                reportError("iTable::initialSetup - iline was bad\n");
                return 0;
            }
            if (in->unk_30) {
                if (in->unk_30 != (cls_unk30 *)this) {
                    reportError("iTable::prep - didn't own this instruction\n");
                    return 0;
                }
            } else if (!in->unk_1428) {
                reportError("iTable::prep - instruction not a move, but owner wasn't set\n");
                return 0;
            }
            in->unk_30 = 0;
            in->setOwner((cls_unk30 *)this);
            if (!logAddLinear_Tail(in)) {
                extern const char prepRawAddInstrError[];
                reportError(prepRawAddInstrError);
                in->leDump();
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
