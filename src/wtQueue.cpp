#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int unk_10;
};

extern optimiser optTable[11];
const char *g_080de730;

instStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, instStream *s, bool b);
workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(workTable *wt);
bool func_0807f360(workTable *wt, wrangleFunc f);
bool func_0808061c(workTable *wt, wrangleFunc f);
bool reduceExtents(workTable *wt);
bool fiddleBDL(workTable *wt);
bool assignLocalRegisters(Program *p, workTable *wt);
bool makeLpWork(instStream *s, int maxSpread);
bool disposeLpWork(instStream *s);
bool keepLpWork(instStream *s);
bool optReset(Program *p);
workTable *makeForceTable(Program *p, instStream *s);
void func_0806ed78(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(workTable *wt);
bool func_0807e528(Program *p, instStream *s, workTable *wt);
bool func_0807e740(Program *p, workTable *wt, unsigned char a, unsigned char b);

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

bool setPipeBits(Instr *in);
bool AddToDo_Tail(Block *blk, Instr *n);
extern int g_080b33f0;

struct wrangleCand;
bool func_080817d4(workTable *wt, wrangleCand *c);
bool func_0808184c(workTable *wt, wrangleCand *c);
bool optNormal1(workTable *wt, wrangleCand *c);
bool optHDown(workTable *wt, wrangleCand *c);
bool optVuta(workTable *wt, wrangleCand *c);
bool optDUp(workTable *wt, wrangleCand *c);
bool optNormal(workTable *wt, wrangleCand *c);
bool optAnother(workTable *wt, wrangleCand *c);
bool optVuta1(workTable *wt, wrangleCand *c);
bool optNormal2(workTable *wt, wrangleCand *c);
bool optDumb2(workTable *wt, wrangleCand *c);

optimiser optTable[11] = {
    {"dumb", 1, 3, (wrangleFunc)func_080817d4, 0},
    {"ldumb", 1, 3, (wrangleFunc)func_0808184c, 0},
    {"normal1", 1, 1, (wrangleFunc)optNormal1, 0},
    {"hDown", 1, 1, (wrangleFunc)optHDown, 0},
    {"vuta", 1, 3, (wrangleFunc)optVuta, 0},
    {"dUp", 1, 1, (wrangleFunc)optDUp, 0},
    {"normal", 1, 3, (wrangleFunc)optNormal, 0},
    {"another", 1, 3, (wrangleFunc)optAnother, 0},
    {"vuta1", 0, 3, (wrangleFunc)optVuta1, 0},
    {"normal2", 0, 3, (wrangleFunc)optNormal2, 0},
    {"dumb2", 0, 1, (wrangleFunc)optDumb2, 0},
};

bool workTable::qInstruction(Instr *in)
{
    int i;
    if (!in->func_080541fc()) {
        func_0804ad38("tried to queue a synthetic\n");
        return false;
    }
    in->func_08054138();
    Instr *c = in->clone();
    if (!c) {
        func_0804ad38("failed to clone instruction\n");
        func_0804adf0(-1);
    }
    c->func_08055f78(in);
    if (!unk_10) {
        func_0804ad38("qInstruction - no owner\n");
        return false;
    }
    if (c->unk_13d5 == 1 || c->unk_13d5 == 2) {
        if (c->unk_13d6 < 0) {
            func_0804ad38("m_aaOp<0 but dstMode is set!\n");
            func_0804adf0(-1);
        }
        c->unk_38[c->unk_13d6].unk_2b = 2;
    }
    setPipeBits(c);

    for (i = 0; i != c->unk_138b; i++) {
        Operand *src = in->getOp(i);
        Operand *dst = c->getOp(i);
        if (!src || !dst) {
            func_0804ad38("qInstruction failed to get an operand\n");
            return false;
        }
        RegPoolEntry *reg = dst->unk_18;
        if (!reg)
            continue;
        regUse *rr = unk_14.func_0805b5dc(reg);
        if (!rr) {
            func_0804ad38("find reg ref failed to find rr from owner.. by now - this is fatal.. register %s\n",
                          reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        if (dst->isInRegOpB()) {
            if (dst->unk_2b == 2)
                dst->unk_09 = 3;
        }
        if (dst->isInRegOpB()) {
            if (g_080b336c && func_0806c6c4(reg) && dst->unk_2b != 2)
                dst->unk_2b = 1;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((dst->unk_07 >> k) & 1) {
                cls0805bcc4 *lk = &dst->unk_2c[k];
                int a = 0;
                cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(k);
                if (!ll) {
                    func_0804ad38("failed to get curr lock list during qinst\n");
                    func_0804adf0(-1);
                }
                lk->setOwner(k, c, i);
                cls0805bcc4 *out;
                if ((dst->unk_27 >> k) & 1) {
                    regUse *o = unk_14.func_0805b5dc(reg);
                    if (!o) {
                        func_0804ad38("failed to find regref owner for input\n");
                        return false;
                    }
                    out = o->unk_20[k].func_0805b830();
                } else {
                    ol_s *ol = src->unk_2c[k].unk_54.unk_00;
                    if (!ol || ol->unk_00) {
                        func_0804ad38("more or less than one oploc for non-extern input\n");
                        func_0804adf0(-1);
                    }
                    Instr *setter = ol->unk_04;
                    if (!setter) {
                        func_0804ad38("could not get setting op for setter\n");
                        func_0804adf0(-1);
                    }
                    Instr *img = setter->unk_08;
                    if (!img) {
                        func_0804ad38("qInstruction - setter had null work clone set\n");
                        return false;
                    }
                    if ((workTable *)img->unk_30 != this) {
                        func_0804ad38("qInstruction  - image of setter wasn't in this table\n");
                        return false;
                    }
                    Operand *sop = img->getOp(ol->unk_10);
                    if (!sop) {
                        func_0804ad38("failed to get setter operand\n");
                        return false;
                    }
                    out = &sop->unk_2c[k];
                    a = dst->unk_29;
                }
                lk->unk_19 = dst->unk_2b;
                if (!ll->addInput(out, lk, a, dst->unk_0b)) {
                    func_0804ad38("failed to register input operand\n");
                    return false;
                }
                }
            }
        }
        if (dst->isReg2() && dst->unk_2b != 2 && g_080b336c && func_0806c6c4(reg) && !rr->unk_0d)
            dst->unk_2b = 1;
    }

    for (i = 0; i != c->unk_138b; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = op->outputReg();
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((op->unk_07 >> k) & 1) {
                cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(k);
                if (!ll) {
                    func_0804ad38("couldn't get rll from reg\n");
                    func_0804adf0(-1);
                }
                cls0805bcc4 *lk = &op->unk_2c[k];
                lk->setOwner(k, c, i);
                int a = 1;
                if (reg->unk_2c)
                    a = c->unk_13cf;
                if (!ll->addOutput(lk, a, op->unk_0a, op->unk_0b)) {
                    func_0804ad38("failed to register output operand : %s.%d\n", reg->func_0805a8e0(), k);
                    return false;
                }
                if ((op->unk_27 >> k) & 1) {
                    cls0805c244 *el = (cls0805c244 *)reg->func_0805a908(k);
                    if (!el)
                        func_0804ad38("couldn't get curr lock list for output\n");
                    if (!el->addExtOutput(lk, false)) {
                        func_0804ad38("failed to add ext output!\n");
                        return false;
                    }
                }
            }
        }
    }

    for (i = 0; i != c->unk_138b; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = op->inputRegB();
        if (!reg)
            continue;
        Operand *o2;
        for (int j = 0; (o2 = c->getOp(j)) != 0; j++) {
            if (o2->getReg2() != reg)
                continue;
            for (int k = 0; k != reg->unk_28; k++) {
                if (((op->unk_07 & o2->unk_07) >> k) & 1) {
                    if (!op->unk_2c[k].setInOut(&o2->unk_2c[k])) {
                        func_0804ad38("failed to set in out\n");
                        func_0804adf0(-1);
                        return false;
                    }
                }
            }
        }
    }

    if (!AddToDo_Tail((Block *)this, c)) {
        func_0804ad38("iTable::qInstruction - failed to add to todo tail\n");
        delete c;
        return false;
    }
    if (!logAddLinear_Tail(c)) {
        func_0804ad38("iTable::qInstruction - failed to add to linear list tail\n");
        return false;
    }
    if (unk_38 && c->unk_1388) {
        g_080b33f0 += 2;
        c->unk_1398 = g_080b33f0;
        reserveEntry(g_080b33f0, unk_38 - 2, 1);
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
                in->func_08054008(false);
                return false;
            }
        } else {
            if (c->unk_00) {
                func_0804ad38("doPostLinear() - original had m_optClone set\n");
                c->func_08054008(false);
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

int getNumOptimisers()
{
    return 11;
}

bool doLinear(Program *p, int smart)
{
    instStream *s;
    while ((s = selectUninsertedStream(p)) != 0) {
        if (s->func_0806d908() != 1) {
            func_0804ad38("doLinear() - asked to process a non-uta\n");
            return false;
        }
        if (s->getAllocMode() != 2) {
            func_0804ad38("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                          s->func_0806d674(), s->getAllocMode());
            return false;
        }
        if (!prepExtInfo(p, s, 0)) {
            func_0804ad38("failed to prep external info for stream\n");
            return false;
        }
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("flowMon::Optimise() - couldn't get work table for %s!\n", s->func_0806d674());
            return false;
        }
        if (wt->func_080741b4()) {
            func_0804ad38("loop size set for linear table opt %s\n", s->func_0806d674());
            return false;
        }
        workTable *best = 0;
        int i = smart ? 10 : 0;
        for (; i >= 0 && (i != 0 || !best); i--) {
            optimiser *o = &optTable[i];
            if (o->flags & 2) {
                g_080de730 = o->name;
                workTable *nt = prepUta(p, s, 0, 1);
                if (!nt) {
                    func_0804ad38("failed to prepUta istream\n");
                    return false;
                }
                nt->unk_00 = i;
                if (!statReset(p)) {
                    func_0804ad38("failed to reset stats for cpu\n");
                    return false;
                }
                if (!dynCalcStats(nt)) {
                    func_0804ad38("dyn calc stats failed for linear\n");
                    return false;
                }
                bool ok = false;
                switch (o->wmode) {
                case 0:
                    ok = func_0808061c(nt, o->func);
                    break;
                case 1:
                    ok = func_0807f360(nt, o->func);
                    break;
                default:
                    func_0804ad38("bad wmode\n");
                    func_0804aeb0();
                }
                if (!ok)
                    func_0804ac7c(" WARNING: failed to %s via %s\n", o->name, s->func_0806d674());
                else {
                    if (!reduceExtents(nt)) {
                        func_0804ad38("failed to reduce extents for code!\n");
                        return false;
                    }
                    if (!fiddleBDL(nt)) {
                        func_0804ad38("reduceEx - failed to fiddle bdl\n");
                        return false;
                    }
                    if (!nt->doPostOpt(1)) {
                        func_0804ad38("failed to validate/patch moves - so continue\n");
                        func_0804adf0(-1);
                    }
                    if (assignLocalRegisters(p, nt)) {
                        if (best) {
                            if (nt->getPhysicalLen() < best->getPhysicalLen()) {
                                delete best;
                                best = nt;
                            } else
                                delete nt;
                        } else
                            best = nt;
                        if (best && best->getPhysicalLen() <= s->func_0805f284())
                            break;
                        continue;
                    }
                }
                delete nt;
            }
        }
        if (!best) {
            func_0804ad38("no opt table.. something failed making table .. for %s!\n", s->func_0806d674());
            s->dump(0);
            return false;
        }
        int len = best->getPhysicalLen();
        if (len < 0) {
            func_0804ad38("optTable was bad size\n");
            return false;
        }
        if (!s->setOptTable(best)) {
            func_0804ad38("set opt table - failed \n");
            return false;
        }
        func_0804acb0("Opt::Linear %16s  %d/%d\n", s->func_0806d674(), len, s->func_0805f284());
    }
    return true;
}

bool doLoop(Program *p, bool smart)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        int i;
        if (!s->canLoop() || !s->unk_270)
            continue;
        workTable *best = 0;
        if (!prepExtInfo(p, s, 1)) {
            func_0804ad38("failed to prep external info for stream\n");
            return false;
        }
        int size = s->func_0805f29c();
        size += s->unk_26c;
        if (s->unk_268 > 1) {
            if (size > s->unk_268)
                func_0804ac7c("the minimal possible loop size + --LoopExtra was greather than the --LoopAbs setting, so ignoring --LoopAbs\n");
            else
                size = s->unk_268;
        }
        bool found = false;
        do {
            s->unk_00 = 11;
            if (!makeLpWork(s, size)) {
                func_0804ad38("failed to generate a work table for loop size %d\n", size);
                return false;
            }
            bool first = true;
            for (i = 10; i >= 0; i--) {
                s->unk_04[i].unk_04 = -1;
                s->unk_04[i].unk_00 = -1;
                s->unk_04[i].unk_0c = -1;
                s->unk_04[i].unk_08 = -1;
                g_080de730 = optTable[i].name;
                if (optTable[i].flags & 1) {
                    const char *why = stallReasonStr(s->unk_2b8);
                    func_0804acb0("lpgen[%d](limit: %s) - '%s' flow %s\n",
                                  size, why, optTable[i].name, s->func_0806d674());
                    workTable *nt = prepUta(p, s, size, 1);
                    if (!nt) {
                        func_0804ad38("failed to prepUta istream\n");
                        return false;
                    }
                    nt->unk_00 = i;
                    if (!statReset(p)) {
                        func_0804ad38("failed to reset stats for cpu\n");
                        return false;
                    }
                    if (!dynCalcStats(nt)) {
                        func_0804ad38("failed to calc stats for loop\n");
                        return false;
                    }
                    if (first)
                        first = false;
                    bool ok = false;
                    switch (optTable[i].wmode) {
                    case 0:
                        ok = func_0808061c(nt, optTable[i].func);
                        break;
                    case 1:
                        ok = func_0807f360(nt, optTable[i].func);
                        break;
                    default:
                        func_0804ad38("bad wmode\n");
                        func_0804aeb0();
                    }
                    if (ok) {
                        if (!reduceExtents(nt)) {
                            func_0804ad38("failed to reduce extents for code!\n");
                            func_0804adf0(-1);
                        }
                        if (!nt->doPostOpt(2)) {
                            func_0804ad38("failed to validate/patch moves - so continue\n");
                            func_0804adf0(-1);
                        }
                        if (!assignLocalRegisters(p, nt))
                            func_0804ac7c("failed to assign reg size would be %d!\n", nt->func_080741b4());
                        else {
                            s->unk_04[i].unk_04 = s->func_0805f29c();
                            s->unk_04[i].unk_08 = nt->unk_3c;
                            s->unk_04[i].unk_0c = nt->getCondLid();
                            s->unk_04[i].unk_00 = nt->func_080741b4();
                            if (best && best->unk_3c < nt->unk_3c) {
                                delete nt;
                                nt = 0;
                            }
                            if (nt) {
                                if (best)
                                    delete best;
                                found = true;
                                best = nt;
                            }
                            continue;
                        }
                    }
                    delete nt;
                }
            }
            if (!found) {
                if (!disposeLpWork(s)) {
                    func_0804ad38("failed to unget work for loop table\n");
                    return false;
                }
            } else if (!keepLpWork(s)) {
                func_0804ad38("failed to fix the new work table for loop opt\n");
                return false;
            }
            size++;
        } while (!found);
        if (!best) {
            func_0804ad38("loop code failed for %s\n", s->func_0806d674());
            return false;
        }
        if (!s->setOptTable(best)) {
            func_0804ad38("setOptTable failed\n");
            return false;
        }
        func_0804acb0("Opt::Loop %16s  %d/%d\n", s->func_0806d674(), best->func_080741b4(), s->func_0805f29c());
    }
    return true;
}

bool doRawAndSynth(Program *p, bool smart)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->func_0806d908() == 3) {
            if (!prepExtInfo(p, s, 0)) {
                func_0804ad38("failed to prep external info for stream\n");
                return false;
            }
            if (!s->getTable()) {
                func_0804ad38("work table not set\n");
                func_0804adf0(-1);
            }
            if (!optReset(p)) {
                func_0804ad38("failed to optReset cpu\n");
                func_0804adf0(-1);
            }
            workTable *wt = makeForceTable(p, s);
            if (!wt) {
                s->dump(0);
                func_0804ad38("failed to prepUta istream\n");
                return false;
            }
            if (!wt->doPostOpt(s->func_0806d968())) {
                func_0804ad38("failed to post opt after makeForce\n");
                return false;
            }
            if (wt->getPhysicalLen() < 0) {
                func_0804ad38("optTable was bad size\n");
                return false;
            }
            switch ((unsigned char)wt->unk_08) {
            case 2:
                if (!assignLocalRegisters(p, wt)) {
                    func_0804ad38("failed to assign reg for vsm table!\n");
                    func_0806ed78(p, 0);
                    func_0804ad38("here is %s\n", s->func_0806d674());
                    return false;
                }
                /* fall through */
            case 3:
                if (!s->setOptTable(wt)) {
                    func_0804ad38("set opt table - failed \n");
                    return false;
                }
                break;
            default:
                func_0804ad38("bad alloc mode in dorawandsynth\n");
                return false;
            }
        }
        if (s->func_0806d908() == 4) {
            workTable *wt = s->getTable();
            if (s->getAllocMode() != 2) {
                func_0804ad38("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                              s->func_0806d674(), s->getAllocMode());
                return false;
            }
            wt->unk_08 = 3;
        }
    }
    return true;
}

bool relaxOpt(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        switch (s->func_0806d908()) {
        case 3: {
            workTable *opt = s->unk_214;
            if (!opt) {
                func_0804ad38("relaxopt - no opt for %s\n", s->func_0806d674());
                return false;
            }
            workTable *wt = s->getTable();
            if (opt != wt) {
                func_0804ad38("relaxOpt - work and opt not same\n");
                return false;
            }
            wt->dump();
            for (Instr *i = s->getTable()->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
                i->unk_00 = 0;
                i->unk_08 = 0;
                i->unk_04 = 0;
            }
            s->unk_214 = 0;
            break;
        }
        case 4:
            break;
        case 1:
            func_0804ad38("uta found at relax time %s\n", s->func_0806d674());
            return false;
        default:
            func_0804ad38("unknown code mode found at relax time %s\n", s->func_0806d674());
            return false;
        }
    }
    return true;
}

bool func_08079fdc(Program *p, bool smart, bool sizeReduce)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->func_0806d908() == 3) {
            if (!prepExtInfo(p, s, 0)) {
                func_0804ad38("failed to prep external info for stream\n");
                return false;
            }
            if (!optReset(p)) {
                func_0804ad38("failed to optReset cpu\n");
                func_0804adf0(-1);
            }
            workTable *wt = makeForceTable(p, s);
            if (!wt) {
                func_0804ad38("failed to prepUta istream\n");
                return false;
            }
            if (!func_0807e528(p, s, wt)) {
                func_0804ad38("failed to do initial reduction test\n");
                return false;
            }
            if (s->func_0806d9a0()) {
                if (!func_0807e740(p, wt, s->unk_261, s->unk_262)) {
                    func_0804ad38("failed to linear reduce table\n");
                    return false;
                }
                if (!sredTable(wt)) {
                    func_0804ad38("failed to prep for sred for %s\n", s->func_0806d674());
                    return false;
                }
            }
            if (!wt->doPostOpt(s->func_0806d968())) {
                wt->dump();
                func_0804ad38("failed to post opt after makeForce\n");
                return false;
            }
            if (!s->setOptTable(wt)) {
                func_0804ad38("set opt table - failed \n");
                return false;
            }
        }
    }
    return true;
}

const char *getOptimiserName(int id)
{
    int numOptimisers = 11;
    if (id < 0 || id >= numOptimisers) {
        func_0804ad38("asked to get an out of bounds optimiser id\n");
        func_0804aeb0();
    }
    return optTable[id].name;
}
