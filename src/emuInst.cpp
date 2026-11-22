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
    RegPoolEntry *reg;
    if (!row) {
        reportError("passed a null pointer to emuVSM\n");
        return -1;
    }
    if (row->skip)
        return 0;
    if (uta && !Cycle(p, uta)) {
        reportError("Processor::Emu - failed initial cycle for uta!\n");
        return -1;
    }

    for (k = 0; k != 2; k++) {
        Instr *in = row->entries[k].unk_00;
        if (!in)
            continue;
        if (commit && in->previousThroughputWriter) {
            reportError("previous thruput write already set for instruction\n");
            return -1;
        }
        int lid = (signed char)in->position.unk_03;
        if (uta) {
            if (lid != -1) {
                reportError("lid>=0 for uta mode emu\n");
                return -1;
            }
        } else if (lid) {
            reportError("emu - called with an instruction with lid non-zero for vsm..%d\n", lid);
            return -1;
        }
    }
    int maxLat = 0, maxThru = 0;
    for (k = 0; k != 2; k++) {
        in = row->entries[k].unk_00;
        if (!in)
            continue;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            reg = registerOperand(op, 1);
            if (reg) {
                for (int part = 0; part != reg->partCount; part++)
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
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
                for (int part = 0; part != reg->partCount; part++)
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
                        int lat;
                        if (rp->pendingSource && (lat = rp->pendingSource->unk_12) != 0) {
                            ol_s tmp;
                            tmp.init(in, o, part, false);
                            if (commit && !p->proc.stalls.addStall(rp->pendingSource, &tmp, 0, lat)) {
                                reportError("failed to add an internal latency stall\n");
                                return -1;
                            }
                            if (op->additionalLatency) {
                                reportError("vcl detected an ialu op too close to a dependent conditional branch. target behaviour isn't guaranteed\n");
                                in->leDump();
                                return -1;
                            }
                            if (lat > maxLat)
                                maxLat = lat;
                        }
                    }
            }
            reg = registerOperand(op, 2);
            if (reg) {
                if (op->componentMask) {
                    if (reg->unk_2c) {
                        if (commit) {
                            if (in->previousThroughputWriter) {
                                reportError("multiple set for prevThruput\n");
                                return -1;
                            }
                            in->previousThroughputWriter = reg->throughputWriter;
                        } else if (reg->throughputWriter != in->previousThroughputWriter) {
                            reportError("unexpected value found in prev thru write\n");
                            return -1;
                        }
                    }
                    reg->throughputWriter = in;
                    reg->unk_36++;
                }
                for (int part = 0; part != reg->partCount; part++) {
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
                        if (rp->issueLock) {
                            ol_s tmp;
                            tmp.init(in, o, part, false);
                            if (commit) {
                                int issueLock = rp->issueLock;
                                if (!p->proc.stalls.addStall(rp->pendingSource, &tmp, 1, issueLock)) {
                                    reportError("failed to add an internal thruput stall\n");
                                    return -1;
                                }
                            }
                            maxThru = rp->issueLock;
                        }
                    }
                }
            }
        }
    }
    int cyc;
    if (uta) {
        if (maxLat || maxThru) {
            reportError("in uta mode, but encountered latency / throughput stall\n");
            return -1;
        }
        cyc = -1;
    } else {
        int stall = 0;
        if (maxLat)
            stall = maxLat;
        if (maxThru && maxThru > stall)
            stall = maxThru;
        if (stall) {
            stalls = stall;
            do {
                if (!Cycle(p, uta)) {
                    reportError("failed to cycle cpu\n");
                    exitAfterCleanup(-1);
                }
            } while (--stall);
        }
        cyc = p->unk_80;
    }
    for (k = 0; k != 2; k++)
        if (row->entries[k].unk_00)
            row->entries[k].unk_00->scheduledCycle = cyc;

    for (k = 0; k != 2; k++) {
        in = row->entries[k].unk_00;
        if (!in)
            continue;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            reg = registerOperand(op, 1);
            if (reg) {
                for (int part = 0; part != reg->partCount; part++) {
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
                        ol_s *setter = &rp->currentLocation;
                        Instr *src = setter->instruction;
                        if (!src) {
                            op->externalPartMask |= 1 << part;
                            continue;
                        }
                        Operand *so = operandAtLocation(setter);
                        if (!so || !isRegisterOperand(so, 2)) {
                            reportError("emuUTA: operand that wasn't output set input\n");
                            return -1;
                        }
                        if (!((so->componentMask >> part) & 1)) {
                            reportError("we used something not really set by setter\n");
                            return -1;
                        }
                        if (commit) {
                            int sourceOp = rp->currentLocation.operandIndex;
                            if (!link_oParts(src, sourceOp, part, in, o, part)) {
                                reportError("failed to cross link operands\n");
                                return -1;
                            }
                        }
                    }
                }
            }
        }
    }
    for (k = 0; k != 2; k++) {
        Instr *in = row->entries[k].unk_00;
        if (!in)
            continue;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            reg = registerOperand(op, 2);
            if (reg) {
                for (int part = 0; part != reg->partCount; part++) {
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
                        if (rp->issueLock) {
                            reportError("inconsistency detected.. issuelock still non-zero\n");
                            return -1;
                        }
                        if (op->unk_0b) {
                            if (!rp->flushPending()) {
                                reportError("failed to flush pending outputs before a blocking output\n");
                                return -1;
                            }
                            ol_s *ol = &rp->currentLocation;
                            ol->init(in, o, part, false);
                            ol->unk_12 = clampByte(op->outputLatency);
                            rp->pendingSource = ol;
                            rp->cycleAge = 0;
                        } else {
                            ol_s *ol = rp->acquireFreePendingLocation();
                            if (!ol) {
                                reportError("EMU: failed to get OL\n");
                                return -1;
                            }
                            ol->init(in, o, part, false);
                            ol->unk_12 = clampByte(op->outputLatency);
                            rp->pendingSource = ol;
                        }
                        rp->unk_00[1] += op->outputLatency;
                        if (op->outputLatency > rp->unk_00[3])
                            rp->unk_00[3] = op->outputLatency;
                        int tp = 1;
                        if (reg->unk_2c)
                            tp = in->baseIssueSpacing;
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
                        rp->unk_00[16] = op->outputLatency;
                        rp->unk_00[15] = tp;
                        rp->unk_00[0] += tp;
                        rp->issueLock = tp;
                    }
                }
            }
        }
    }
    stalls++;
    if (!Cycle(p, uta)) {
        reportError("failed to cycle cpu!\n");
        return -1;
    }
    return stalls;
}
