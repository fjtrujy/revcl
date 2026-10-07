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
    int mode = unk_09;
    if ((unsigned char)mode != 3)
        func_0804ad38("prepRawTable called - but mode is not raw %s\n", codeModeName(mode));
    if (!unk_2c) {
        func_0804ad38("iTable::prepRawTable - table not ready\n");
        return 0;
    }
    if (!setRangeFrom(this)) {
        func_0804ad38("set raw table - failed to set iLoc range\n");
        return 0;
    }
    unk_88.reset();
    unk_a0.reset();
    unk_94.reset();
    unk_4c = 0;
    unk_50 = 0;
    unk_3c = 0;
    resetCounts();
    if (unk_30) {
        func_0804ad38("initial start - table not based at zero\n");
        return 0;
    }
    int maxLid = 0;
    int tlen = unk_34;
    if (tlen < 0) {
        func_0804ad38("prepTable - tlen<0\n");
        return 0;
    }
    if (tlen == 0)
        unk_0a = 1;
    for (int rl = 0; rl < unk_34; rl++) {
        wtRow *ie = (wtRow *)((cls_unk30 *)this)->getIE_ck(rl);
        if (!ie) {
            func_0804ad38("iTable::prepTable - failed to get ientry\n");
            return 0;
        }
        if (ie->unk_08) {
            func_0804ad38("bSkip set for preptabl\n");
            return 0;
        }
        for (int p = 0; p != 2; p++) {
            Instr *in = ie->unk_00[p].unk_00;
            if (!in)
                continue;
            if (in->unk_13cd) {
                func_0804ad38("synth instruction in preptab\n");
                in->leDump();
                func_0804aeb0();
            }
            if (in->unk_1420 != -1) {
                func_0804ad38("parseLid isn't cleared in prepTab\n");
                in->func_08054008(false);
                func_0804adf0(-1);
            }
            int lid = 0;
            int il;
            cls08058718 *loc;
            if (unk_38) {
                if (!in->func_08054138())
                    lid = in->unk_13e8.unk_03;
                if (lid < 0) {
                    func_0804ad38("lid was <0 during table setup\n");
                    in->leDump();
                    return 0;
                }
                loc = &in->unk_13e8;
                loc->unk_03 = lid;
                il = lid * unk_38 + rl;
            } else {
                loc = &in->unk_13e8;
                lid = loc->unk_03;
                if (lid) {
                    func_0804ad38("non zero lid for non-loop %d\n", lid);
                    dumpRawInstruction(in);
                    return 0;
                }
                loc->unk_03 = 0;
                il = rl;
            }
            if (lid > maxLid)
                maxLid = lid;
            cls08058718 pos;
            pos.func_0805874c(il);
            pos.func_0805878c(p);
            in->unk_13e8.unk_00 = pos.unk_00;
            signed char nl = pos.unk_03;
            loc->unk_04 = pos.unk_04;
            loc->unk_03 = nl;
            loc->unk_02 = pos.unk_02;
            if (nl != lid) {
                func_0804ad38("iTable::initialSetup - lid was bad (%d!=%d) lsz=%d rl=%d il=%d\n",
                              nl, lid, unk_38, il, rl);
                return 0;
            }
            if (pos.unk_04 != rl) {
                func_0804ad38("iTable::initialSetup - iline was bad\n");
                return 0;
            }
            if (in->unk_30) {
                if (in->unk_30 != (cls_unk30 *)this) {
                    func_0804ad38("iTable::prep - didn't own this instruction\n");
                    return 0;
                }
            } else if (!in->unk_1428) {
                func_0804ad38("iTable::prep - instruction not a move, but owner wasn't set\n");
                return 0;
            }
            in->unk_30 = 0;
            in->setOwner((cls_unk30 *)this);
            if (!logAddLinear_Tail(in)) {
                func_0804ad38("iTable - failed to add an instruction\n");
                in->leDump();
                return 0;
            }
        }
    }
    if (unk_38)
        unk_3c = maxLid;
    else if (maxLid) {
        func_0804ad38("iTable::initialSetup - maxLid set - but not loopsize!\n");
        return 0;
    }
    resetRange();
    return 1;
}
