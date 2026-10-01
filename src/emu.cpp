#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "ol_s_inline.h"

int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
bool Cycle(Program *p, bool flag);

static inline int clampByte(int v)
{
    return v > 255 ? 255 : v;
}

bool emuTable(Program *p, workTable *wt)
{
    if (!wt) {
        func_0804ac7c("emu - passed a null table ptr!\n");
        return true;
    }
    if (wt->func_080741b4() || wt->unk_3c) {
        func_0804ad38("mlid is set, or loop size is set for emu..which is now kinda tricky..\n");
        return false;
    }
    wt->unk_0c = -1;
    switch (wt->unk_09) {
    case 1:
    case 4: {
        Instr *i = wt->unk_88.unk_00;
        wtRow row;
        for (; i; i = i->unk_1c.unk_00) {
            row.func_080721f8();
            row.unk_00[0].unk_00 = i;
            int r = emuInst(p, &row, -1, true, true);
            row.unk_00[0].unk_00 = 0;
            if (r < 0) {
                func_0804ad38("failed to uta emu instruction\n");
                i->func_08054008(false);
                return false;
            }
        }
        break;
    }
    case 3: {
        int skipped = 0;
        for (int line = wt->func_0807411c(); line != wt->func_080741c0(); line++) {
            wtRow *r = wt->func_080724f0(line);
            if (!r) {
                func_0804ad38("emuuta - failed to get an ie\n");
                return false;
            }
            if (r->unk_08) {
                for (int k = 0; k != 2; k++)
                    if (r->unk_00[k].unk_00) {
                        func_0804ad38("m_bSkip set .. but line contains instructions\n");
                        return false;
                    }
                skipped++;
                continue;
            }
            if (emuInst(p, r, line, false, true) < 0) {
                func_0804ad38("failed to Emu an inst\n");
                r->func_0807222c(-1);
                return false;
            }
        }
        wt->unk_0c = p->unk_80;
        skipped += p->unk_80;
        if (skipped < wt->getPhysicalLen()) {
            func_0804ad38("logical len < physical len.. %d<%d!\n", p->unk_80, wt->getPhysicalLen());
            wt->dump();
            return false;
        }
        break;
    }
    default:
        func_0804ad38("emu - can't emu table from %s of type %s\n",
                      wt->ownerName(), codeModeName(wt->unk_09));
        break;
    }
    return true;
}

bool func_080642ec(Program *p, instStream *s)
{
    int count;
    Instr *i;
    workTable *wt;

    wt = s->getTable();
    int linear = wt->unk_c0[2];
    int iCount = wt->unk_88.unk_08;
    if (linear != iCount) {
        func_0804ad38("linear count not same as iCount %d!=%d\n", linear, iCount);
        return false;
    }
    if (!emuTable(p, wt)) {
        func_0804ad38("Processor : failed to emu itable from '%s'\n", s->func_0806d674());
        return false;
    }
    count = 0;
    for (i = wt->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
        count++;
        for (int op = 0; op != i->unk_138b; op++) {
            Operand *o = &i->unk_38[op];
            RegPoolEntry *reg = o->unk_18;
            if (!reg)
                continue;
            regUse *ru = wt->unk_14.func_0805b5dc(reg);
            if (!ru) {
                ru = wt->unk_14.AddNewRegRef(reg);
                if (!ru) {
                    func_0804ad38("failed to add reg ref\n");
                    return false;
                }
            }
            if (o->isReg2())
                ru->unk_0f |= o->unk_07;
            if (o->isReg())
                ru->unk_0e |= o->unk_07;
            if (o->isReg2()) {
                if (o->unk_27) {
                    func_0804ad38("operand had xbits set for output!\n");
                    return false;
                }
            }
            ru->unk_0c |= o->unk_27;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((o->unk_27 >> k) & 1) {
                    if (ru->unk_15 == -1)
                        ru->unk_15 = o->unk_0b;
                    else if (ru->unk_15 != (unsigned char)o->unk_0b)
                        func_0804ac7c("differing block modes for external input for %s..taking first encountered\n",
                                      reg->func_0805a8e0());
                    if (!ru->unk_20[k].unk_00.func_080580fc(i, op, k, false)) {
                        func_0804ad38("failed to add oploc user!\n");
                        return false;
                    }
                }
                if (((o->unk_07 >> k) & 1) && reg->unk_2c && o->isReg2()) {
                    if (!ru->unk_20[k].unk_20.func_080580fc(i, op, k, false)) {
                        func_0804ad38("failed to add a local setter for a thruput check\n");
                        func_0804ad38("register is %s\n", reg->func_0805a8e0());
                        return false;
                    }
                }
            }
        }
    }
    if (count != wt->unk_c0[2]) {
        func_0804ad38("icount mismatch (%d apparently in list, but %d in table?)\n", count, wt->unk_c0[2]);
        return false;
    }
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            func_0804ad38("a reg ref had no register!\n");
            return false;
        }
        for (int part = 0; part != reg->unk_28; part++) {
            rPart *rp = &reg->unk_44[part];
            regUseSlot *slot = &ru->unk_20[part];
            slot->unk_3c[0] = rp->unk_00[4];
            slot->unk_3c[1] = rp->unk_00[5];
            slot->unk_3c[2] = rp->unk_00[6];
            slot->unk_3c[3] = rp->unk_00[7];
            slot->unk_3c[4] = rp->unk_00[8];
            slot->unk_3c[5] = rp->unk_00[9];
            slot->unk_3c[6] = rp->unk_00[10];
            slot->unk_3c[7] = rp->unk_00[11];
            slot->unk_3c[8] = rp->unk_00[12];
            slot->unk_3c[9] = rp->unk_00[13];
            slot->unk_3c[10] = rp->unk_00[14];
            slot->unk_3c[11] = rp->unk_00[15];
            slot->unk_3c[12] = rp->unk_00[16];
            slot->unk_34[1] = rp->unk_00[1];
            slot->unk_34[0] = rp->unk_00[0];
            slot->unk_34[3] = rp->unk_00[2];
            slot->unk_34[2] = rp->unk_00[3];
            if ((ru->unk_0f >> part) & 1) {
                ol_s *out = &rp->unk_24;
                if (out->unk_04 && !slot->addOutput(out)) {
                    func_0804ad38("failed to add an output to an rrp\n");
                    return false;
                }
                for (int j = 0; j != 4; j++) {
                    ol_s *ol = rp->func_0805a968(j);
                    if (!ol) {
                        func_0804ad38("failed to get a pool entry\n");
                        return false;
                    }
                    if (ol->unk_12 && !slot->addOutput(ol)) {
                        func_0804ad38("failed to add a pending output to an rrp\n");
                        return false;
                    }
                }
                if (slot->unk_08.unk_04 <= 0) {
                    func_0804ad38("no or -ve outputs during out build..nOut=%d!\n", slot->unk_08.unk_04);
                    return false;
                }
            }
        }
    }
    return true;
}

int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit)
{
    int k;
    int stalls = 0;
    Instr *in;
    Operand *op;
    if (!row) {
        func_0804ad38("passed a null pointer to emuVSM\n");
        return -1;
    }
    if (row->unk_08)
        return 0;
    if (uta && !Cycle(p, uta)) {
        func_0804ad38("Processor::Emu - failed initial cycle for uta!\n");
        return -1;
    }

    for (k = 0; k != 2; k++) {
        Instr *in = row->unk_00[k].unk_00;
        if (!in)
            continue;
        if (commit && in->unk_1434) {
            func_0804ad38("previous thruput write already set for instruction\n");
            return -1;
        }
        int lid = (signed char)in->unk_13e8.unk_03;
        if (uta) {
            if (lid != -1) {
                func_0804ad38("lid>=0 for uta mode emu\n");
                return -1;
            }
        } else if (lid) {
            func_0804ad38("emu - called with an instruction with lid non-zero for vsm..%d\n", lid);
            return -1;
        }
    }
    int maxLat = 0, maxThru = 0;
    for (k = 0; k != 2; k++) {
        in = row->unk_00[k].unk_00;
        if (!in)
            continue;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->inputReg();
            if (reg) {
                for (int part = 0; part != reg->unk_28; part++)
                    if ((op->unk_07 >> part) & 1) {
                        rPart *rp = &reg->unk_44[part];
                        if (rp->unk_00[4] < 0) {
                            if (uta)
                                rp->unk_00[6] = -1;
                            else
                                rp->unk_00[6] = p->unk_80;
                            rp->unk_00[4] = line;
                            rp->unk_00[5] = k;
                        }
                        rp->unk_00[7] = line;
                        rp->unk_00[8] = k;
                    }
            }
            if (reg && op->unk_0b) {
                for (int part = 0; part != reg->unk_28; part++)
                    if ((op->unk_07 >> part) & 1) {
                        rPart *rp = &reg->unk_44[part];
                        int lat;
                        if (rp->unk_38 && (lat = rp->unk_38->unk_12) != 0) {
                            ol_s tmp;
                            tmp.init(in, o, part, false);
                            if (commit && !p->proc.unk_74.addStall(rp->unk_38, &tmp, 0, lat)) {
                                func_0804ad38("failed to add an internal latency stall\n");
                                return -1;
                            }
                            if (op->unk_29) {
                                func_0804ad38("vcl detected an ialu op too close to a dependent conditional branch. target behaviour isn't guaranteed\n");
                                in->leDump();
                                return -1;
                            }
                            if (lat > maxLat)
                                maxLat = lat;
                        }
                    }
            }
            RegPoolEntry *out = op->outRegOp();
            if (out) {
                if (op->unk_07) {
                    if (out->unk_2c) {
                        if (commit) {
                            if (in->unk_1434) {
                                func_0804ad38("multiple set for prevThruput\n");
                                return -1;
                            }
                            in->unk_1434 = out->unk_38;
                        } else if (out->unk_38 != in->unk_1434) {
                            func_0804ad38("unexpected value found in prev thru write\n");
                            return -1;
                        }
                    }
                    out->unk_38 = in;
                    out->unk_36++;
                }
                for (int part = 0; part != out->unk_28; part++) {
                    rPart *rp = &out->unk_44[part];
                    if (((op->unk_07 >> part) & 1) && rp->unk_3e) {
                        ol_s tmp;
                        tmp.init(in, o, part, false);
                        if (commit && !p->proc.unk_74.addStall(rp->unk_38, &tmp, 1, rp->unk_3e)) {
                            func_0804ad38("failed to add an internal thruput stall\n");
                            return -1;
                        }
                        maxThru = rp->unk_3e;
                    }
                }
            }
        }
    }
    int cyc;
    if (uta) {
        if (maxLat || maxThru) {
            func_0804ad38("in uta mode, but encountered latency / throughput stall\n");
            return -1;
        }
        cyc = -1;
    } else {
        int stall = maxLat ? maxLat : 0;
        if (maxThru && maxThru > stall)
            stall = maxThru;
        if (stall) {
            stalls = stall;
            do {
                if (!Cycle(p, uta)) {
                    func_0804ad38("failed to cycle cpu\n");
                    func_0804adf0(-1);
                }
            } while (--stall);
        }
        cyc = p->unk_80;
    }
    for (k = 0; k != 2; k++)
        if (row->unk_00[k].unk_00)
            row->unk_00[k].unk_00->unk_1394 = cyc;

    for (k = 0; k != 2; k++) {
        in = row->unk_00[k].unk_00;
        if (!in)
            continue;
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->inRegOp();
            if (!reg)
                continue;
            for (int part = 0; part != reg->unk_28; part++) {
                if (!((op->unk_07 >> part) & 1))
                    continue;
                rPart *rp = &reg->unk_44[part];
                Instr *src = rp->unk_24.unk_04;
                if (!src) {
                    op->unk_27 |= 1 << part;
                    continue;
                }
                Operand *so = func_0805802c(&rp->unk_24);
                if (!so || !so->isOutRegOp()) {
                    func_0804ad38("emuUTA: operand that wasn't output set input\n");
                    return -1;
                }
                if (!((so->unk_07 >> part) & 1)) {
                    func_0804ad38("we used something not really set by setter\n");
                    return -1;
                }
                if (commit && !link_oParts(src, rp->unk_24.unk_10, part, in, o, part)) {
                    func_0804ad38("failed to cross link operands\n");
                    return -1;
                }
            }
        }
    }
    for (k = 0; k != 2; k++) {
        in = row->unk_00[k].unk_00;
        if (!in)
            continue;
        Operand *op;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            RegPoolEntry *reg = op->outRegOp();
            if (!reg)
                continue;
            for (int part = 0; part != reg->unk_28; part++) {
                if (!((op->unk_07 >> part) & 1))
                    continue;
                rPart *rp = &reg->unk_44[part];
                if (rp->unk_3e) {
                    func_0804ad38("inconsistency detected.. issuelock still non-zero\n");
                    return -1;
                }
                if (op->unk_0b) {
                    if (!rp->flushPending()) {
                        func_0804ad38("failed to flush pending outputs before a blocking output\n");
                        return -1;
                    }
                    rp->unk_24.init(in, o, part, false);
                    rp->unk_24.unk_12 = clampByte(op->unk_0a);
                    rp->unk_38 = &rp->unk_24;
                    rp->unk_3c = 0;
                } else {
                    ol_s *ol = rp->func_0805a164();
                    if (!ol) {
                        func_0804ad38("EMU: failed to get OL\n");
                        return -1;
                    }
                    ol->init(in, o, part, false);
                    ol->unk_12 = clampByte(op->unk_0a);
                    rp->unk_38 = ol;
                }
                rp->unk_00[1] += op->unk_0a;
                if (op->unk_0a > rp->unk_00[3])
                    rp->unk_00[3] = op->unk_0a;
                int tp = 1;
                if (reg->unk_2c)
                    tp = in->unk_13cf;
                if (tp > rp->unk_00[2])
                    rp->unk_00[2] = tp;
                if (rp->unk_00[9] < 0) {
                    rp->unk_00[9] = line;
                    rp->unk_00[10] = k;
                    if (uta)
                        rp->unk_00[11] = -1;
                    else
                        rp->unk_00[11] = p->unk_80;
                }
                rp->unk_00[12] = line;
                rp->unk_00[13] = k;
                if (uta)
                    rp->unk_00[14] = -1;
                else
                    rp->unk_00[14] = p->unk_80;
                rp->unk_00[16] = op->unk_0a;
                rp->unk_00[15] = tp;
                rp->unk_00[0] += tp;
                rp->unk_3e = tp;
            }
        }
    }
    stalls++;
    if (!Cycle(p, uta)) {
        func_0804ad38("failed to cycle cpu!\n");
        return -1;
    }
    return stalls;
}
