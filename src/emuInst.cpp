#include "emu_shared.h"

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "ol_s_inline.h"

int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
bool Cycle(Program *p, bool flag);


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
