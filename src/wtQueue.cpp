#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"

bool workTable::qRegRef(regUse *ru)
{
    if (!ru) {
        func_0804ad38("qRegRef: passed a null pointer\n");
        func_0804adf0(-1);
    }
    regUse *c = ru->Clone();
    if (!c) {
        func_0804ad38("failed to clone regref\n");
        func_0804adf0(-1);
    }
    RegPoolEntry *reg = c->unk_08;
    if (!reg) {
        func_0804ad38("no register attached to cloned rf during qRegRef\n");
        delete c;
        return false;
    }
    if (!unk_14.append(c)) {
        func_0804ad38("failed to add cloned regref to list\n");
        delete c;
        return false;
    }
    if (~c->unk_0f & c->unk_0d) {
        func_0804ad38("xobits set, but o bits not\n");
        c->func_0805b51c();
        return false;
    }
    if (~c->unk_0e & c->unk_0c) {
        func_0804ad38("xibits set, but i bits not\n");
        c->func_0805b51c();
        return false;
    }
    for (int i = 0; i != reg->unk_28; i++) {
        c->unk_20[i].unk_30 = ru->unk_20[i].unk_30;
        c->unk_20[i].unk_32 = ru->unk_20[i].unk_32;
        cls0805c244 *ll = getLockList(reg, i);
        if (!ll) {
            func_0804ad38("failed to get lock list ptr in register for rfv for reg %s\n", reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        if (c->unk_20[i].unk_58) {
            func_0804ad38("rll is already set in rrp\n");
            func_0804adf0(-1);
        }
        c->unk_20[i].unk_58 = (int)ll;
        if (reg->func_0805a908(i)) {
            func_0804ad38("regref register part already had locklist\n");
            func_0804adf0(-1);
        }
        if (!reg->func_0805a924(i, (int)ll)) {
            func_0804ad38("failed to set curr lock list pointer in rf register\n");
            func_0804adf0(-1);
        }
        if ((c->unk_0f >> i) & 1)
            ll->unk_29 = 0;
        char xi = (c->unk_0c >> i) & 1;
        char t = (c->unk_10 >> i) & 1;
        bool sync = false;
        if (reg->unk_2c && g_080b336c == 0)
            sync = true;
        char nb = reg->unk_1c;
        if (!nb && (c->unk_10 & (c->unk_0f | c->unk_0d))) {
            func_0804ad38("t bits set for nbid register\n");
            c->func_0805b51c();
            return false;
        }
        if (t || xi || sync) {
            unsigned char nbz = nb == 0;
            cls0805bcc4 *lk = c->unk_20[i].func_0805b830();
            if (!lk)
                func_0804ad38("failed to get lock for %s.%d as %s\n",
                              reg->func_0805a8e0(), i, reg->func_0805a8e0());
            if (!lk->setOwner(i, c)) {
                func_0804ad38("falied to set owner\n");
                func_0804adf0(-1);
                return false;
            }
            if (!ll->addOutput(lk, 0, 1, nbz)) {
                func_0804ad38("failed to register output lock for regref\n");
                func_0804adf0(-1);
                return false;
            }
            if (t) {
                bool ext = !(((c->unk_0f | c->unk_0e) >> i) & 1);
                if (!ll->addExtOutput(lk, ext)) {
                    func_0804ad38("failed to add pass thru reg ref as ext output for register %s\n",
                                  reg->func_0805a8e0());
                    return false;
                }
                dump();
            }
            if (!ll->addOutputLock(lk)) {
                func_0804ad38("regRef::LoadLocks - failed to add output lock\n");
                return false;
            }
        }
    }
    return true;
}

bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);

bool workTable::forceInsert(Instr *in, cls08058718 *pos)
{
    int i = 0;
    if (!in) {
        func_0804ad38("forceInsert - null in\n");
        return false;
    }
    cls08058718 *p = &in->unk_13e8;
    int row = pos->unk_04;
    int pipe = pos->unk_02;
    if (p->unk_04 >= 0) {
        func_0804ad38("instruction already inserted\n");
        return false;
    }
    if (row < unk_30) {
        func_0804ad38("ForceInsert - before start\n");
        return false;
    }
    if (row >= unk_34) {
        func_0804ad38("ForceInsert - after end\n");
        return false;
    }
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
    if (!ie) {
        func_0804ad38("couldn't get ie\n");
        return false;
    }
    if (pipe < 0 || pipe > 1) {
        func_0804ad38("out of bounds pipe\n");
        return false;
    }
    Instr **slot = &ie[pipe];
    if (*slot != (Instr *)in->unk_1398) {
        func_0804ad38("instruction already inserted or bad key\n");
        return false;
    }
    p->unk_00 = pos->unk_00;
    p->unk_04 = pos->unk_04;
    p->unk_03 = pos->unk_03;
    p->unk_02 = pos->unk_02;
    *slot = in;
    in->unk_1396 = unk_40;
    unk_40++;
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *reg = op->isInReg() ? op->unk_18 : 0;
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((bool)((op->unk_07 >> k) & 1)) {
                cls0805bcc4 *lk = &op->unk_2c[k];
                if (!lk) {
                    func_0804ad38("failed to get lock \n");
                    return false;
                }
                cls0805c244 *ll = lk->unk_04;
                if (!ll) {
                    func_0804ad38("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addInputLock(lk)) {
                    func_0804ad38("failed to add input lock\n");
                    return false;
                }
            }
        }
    }
    for (i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        RegPoolEntry *reg = op->isOutReg() ? op->unk_18 : 0;
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((bool)((op->unk_07 >> k) & 1)) {
                cls0805bcc4 *lk = &op->unk_2c[k];
                if (!lk) {
                    func_0804ad38("failed to get lock \n");
                    return false;
                }
                cls0805c244 *ll = lk->unk_04;
                if (!ll) {
                    func_0804ad38("lock had no lock list\n");
                    return false;
                }
                lk->setFrom(pos);
                if (!ll->addOutputLock(lk)) {
                    func_0804ad38("failed to add output lock\n");
                    return false;
                }
            }
        }
    }
    if (!UnlinkFromToDo((Block *)this, in)) {
        func_0804ad38("failed to unlink from the todo list\n");
        return false;
    }
    if (!AddDone_Tail((Block *)this, in)) {
        func_0804ad38("failed to add instruction to the done list tail\n");
        return false;
    }
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n);
bool setRange(int s, int e, int lsz);

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
                    case 2:
                        in->unk_1394 = in->unk_13e8.unk_04;
                        if (unk_30) {
                            func_0804ad38("postopt - istart set for fixed mode \n");
                            return false;
                        }
                        break;
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
                if (ie[p] && !logAddLinear_Tail(ie[p])) {
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
