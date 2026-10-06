#include "wtQueue_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);


bool workTable::doPostOpt(int rawMode)
{
    if (!unk_2c) {
        func_0804ac7c("doPostOpt() - no attached table!\n");
        return true;
    }
    {
        if (unk_30 < 0 || unk_34 < 0 || unk_34 < unk_30) {
            func_0804ad38("bad table start/end\n");
            return false;
        }
        if (unk_09 != 3) {
            func_0804ad38("not mid-code mode for doPostOpt\n");
            return false;
        }
        unk_0b = 1;
        if (unk_94.unk_08) {
            func_0804ad38("doPostOpt - but there's things still to do\n");
            return false;
        }
        instStream *owner = unk_10;
        if (!owner) {
            func_0804ad38("doPostOpt - not get owner\n");
            return false;
        }
        workTable *pw = owner->getTable();
        if (!pw) {
            func_0804ad38("failed to get work table from owner\n");
            return false;
        }
        if (pw == this) {
            func_0804ad38("doPostOpt ..parent work table is us!\n");
            return false;
        }
        int expected;
        wtInstrList *lin = &unk_88;
        expected = lin->unk_08;
        int count = 0;
        int et = owner->unk_2cd;
        if (et == 2) {
            if (unk_34 - unk_30 <= 1)
                unk_34 = unk_30 + 2;
        } else if (et == 3) {
            if (unk_34 - unk_30 <= 1)
                unk_34 = unk_30 + 2;
        }
        ::setRange(unk_30, unk_34, unk_38);
        int maxLid = 0;
        int len;
        switch (rawMode) {
        case 1:
        case 2:
            len = unk_34 - unk_30;
            break;
        case 3:
        case 4:
            len = pw->func_08072eec();
            break;
        default:
            func_0804ad38("bad raw mode into doPostOpt()\n");
            return false;
        }
        if (!func_08073774(rawMode)) {
            func_0804ad38("doPostOpt : failed to set raw mode..\n");
            return false;
        }
        int rl;
        for (rl = unk_30; rl < unk_34; rl++) {
            Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
            if (!ie) {
                func_0804ad38("iTable::doPostOpt - failed to get ientry\n");
                return false;
            }
            if (((wtRow *)ie)->unk_08) {
                func_0804ad38("bSkip is set\n");
                return false;
            }
            for (int p = 0; p != 2; p++) {
                Instr *in = ie[p];
                if (in) {
                    int nBranch = -1;
                    switch (rawMode) {
                    case 1:
                        in->unk_1394 = in->unk_13e8.unk_04 - unk_30;
                        break;
                    case 2: {
                        int t = in->unk_13e8.unk_04;
                        in->unk_1394 = t;
                        if (unk_30) {
                            func_0804ad38("postopt - istart set for fixed mode \n");
                            return false;
                        }
                        break;
                    }
                    case 3:
                    case 4: {
                        Instr *c = in->unk_08;
                        if (!c) {
                            func_0804ad38("no work clone set\n");
                            return false;
                        }
                        short t = c->unk_1394;
                        if (t < 0 || t >= len) {
                            func_0804ad38("emutick bad for raw postopt\n");
                            return false;
                        }
                        in->unk_1394 = t;
                        break;
                    }
                    default:
                        func_0804ad38("raw post mode for postopt\n");
                        return false;
                    }
                    if (in->unk_1428) {
                        if (in->unk_30) {
                            func_0804ad38("owner not 0 for move\n");
                            return false;
                        }
                    } else {
                        if (in->unk_30 != (cls_unk30 *)this) {
                            func_0804ad38("owner not this for non-move...%p\n", in->unk_30);
                            in->func_08054008(false);
                            return false;
                        }
                        if (!logUnlinkFromLinear(in)) {
                            func_0804ad38("doPostOpt:failed to unlink from linear list\n");
                            return false;
                        }
                        count++;
                    }
                    if (!UnlinkFromDone((Block *)this, in)) {
                        func_0804ad38("doPostOpt -  unlink from done list failed\n");
                        return false;
                    }
                    if (in->unk_1388) {
                        if (++nBranch) {
                            func_0804ad38("doPostOpt - more than one branch\n");
                            return false;
                        }
                        if (rl != unk_34 - 2) {
                            func_0804ad38("badly placed branch detected in postopt\n");
                            dump();
                            return false;
                        }
                    }
                    cls08058718 *loc = &in->unk_13e8;
                    int lid = loc->unk_03;
                    int row = loc->unk_04;
                    int iloc = in->unk_13e8.unk_00;
                    if (!unk_38 && lid) {
                        func_0804ad38("i_lid set not in loop\n");
                        return false;
                    }
                    if (iloc != row + lid * g_080b336c) {
                        func_0804ad38("badly formed iloc in instruction\n");
                        return false;
                    }
                    if (row != rl) {
                        func_0804ad38("instruction not where it thinks it is\n");
                        return false;
                    }
                    if (lid > maxLid)
                        maxLid = lid;
                    if (!ie[p]) {
                        func_0804ad38("damn - lost instruction at %d:%d!\n", rl, p);
                        return false;
                    }
                }
            }
        }
        if (count != expected) {
            func_0804ad38("doPostOpt - linear list inconsistency %d!=%d.. \n", count, expected);
            dump();
            return false;
        }
        if (unk_a0.unk_08) {
            func_0804ad38("doPostOpt() - things remained on done list after scanning table\n");
            return false;
        }
        if (lin->unk_08) {
            func_0804ad38("doPostOpt() - things remained on Linear list after scanning table\n");
            return false;
        }
        if (unk_38)
            unk_3c = maxLid;
        else if (maxLid) {
            func_0804ad38("iTable::doPostOpt - maxLid set - but not loopsize!\n");
            return false;
        }
        resetCounts();
        for (rl = unk_30; rl < unk_34; rl++) {
            Instr **ie = ((cls_unk30 *)this)->getIE_ck(rl);
            if (!ie)
                func_0804ad38("can't get ie during postopt\n");
            for (int p = 0; p != 2; p++) {
                if (!ie[p])
                    continue;
                if (!logAddLinear_Tail(ie[p])) {
                    func_0804ad38("failed to add in to linear tale\n");
                    return false;
                }
            }
        }
        if (unk_38 && unk_34 - unk_30 != unk_38) {
            func_0804ad38("bad loop size for postOpt\n");
            return false;
        }
        unk_0c = len;
    }
    return true;
}

static inline bool isRawTable(workTable *wt)
{
    return wt->unk_09 == 3;
}

static inline void dumpOptInstruction(Instr *in)
{
    in->func_08054008(false);
}

bool instStream::setOptTable(workTable *nt)
{
    if (!nt) {
        func_0804ad38("can't set opt table to null ptr - use clear\n");
        func_0804aeb0();
    }
    if (!isRawTable(nt)) {
        func_0804ad38("setOptTable - table not vsm\n");
        return false;
    }
    workTable *old = getTable();
    if (!old) {
        func_0804ad38("no old work table\n");
        return false;
    }
    if (old == nt) {
        func_0804ad38("old and new are same\n");
        return false;
    }
    if (unk_214) {
        func_0804ad38("stream already has opttable set\n");
        return false;
    }
    for (Instr *in = nt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (in->func_08054168() != this) {
            func_0804ad38("didn't own instruction\n");
            return false;
        }
        if (in->unk_00) {
            func_0804ad38("doPostLinearOpt - optClone set in image?\n");
            return false;
        }
        Instr *c = in->unk_08;
        if (!c) {
            if (!in->unk_1428) {
                func_0804ad38("doPostLinearOpt - inst wasn't cloned, but not a move either\n");
                dumpOptInstruction(in);
                return false;
            }
        } else {
            if (c->unk_00) {
                func_0804ad38("doPostLinear() - original had m_optClone set\n");
                dumpOptInstruction(c);
                return false;
            }
            if (c->func_08054168() != this) {
                func_0804ad38("setOptTable -  cloned image wsn't owned by us\n");
                return false;
            }
            workTable *owner = (workTable *)c->unk_30;
            if (owner != getTable()) {
                func_0804ad38("clone owner wasn't our work table\n");
                return false;
            }
            if (in->unk_138b != c->unk_138b) {
                func_0804ad38("operand count mismatch between original and opt image\n");
                return false;
            }
            for (int i = 0; i != in->unk_138b; i++) {
                char io = in->unk_38[i].unk_09;
                char cio = c->unk_38[i].unk_09;
                if (io != cio) {
                    if (cio != 1 && io != 3) {
                        func_0804ad38("io type mismatch for something that wasn't a whacked input\n");
                        return false;
                    }
                    in->unk_38[i].unk_18 = c->unk_38[i].unk_18;
                    in->unk_38[i].unk_09 = cio;
                }
            }
            in->unk_04 = (int)c;
            c->unk_00 = (int)in;
        }
    }
    nt->clearClones();
    old->clearClones();
    unk_218 = nt;
    unk_214 = nt;
    if (nt->unk_00 >= 0)
        unk_144 |= 1 << nt->unk_00;
    if (nt->func_080741b4())
        unk_140 = 1;
    return true;
}
