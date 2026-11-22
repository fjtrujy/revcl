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


int emuInst(Program *program, wtRow *row, int line, bool uta, bool commit)
{
    int entryIndex;
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
    if (uta && !Cycle(program, uta)) {
        reportError("Processor::Emu - failed initial cycle for uta!\n");
        return -1;
    }

    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        Instr *in = row->entries[entryIndex].instruction;
        if (!in)
            continue;
        if (commit && in->previousThroughputWriter) {
            reportError("previous thruput write already set for instruction\n");
            return -1;
        }
        int lid = (signed char)in->position.loopIteration;
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
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        in = row->entries[entryIndex].instruction;
        if (!in)
            continue;
        for (int o = 0; (op = in->getOp(o)) != 0; o++) {
            reg = registerOperand(op, 1);
            if (reg) {
                for (int part = 0; part != reg->partCount; part++)
                    if ((op->componentMask >> part) & 1) {
                        rPart *registerPart = &reg->parts[part];
                        if (registerPart->emulationTimingMetrics[4] < 0) {
                            if (uta)
                                registerPart->emulationTimingMetrics[6] = -1;
                            else
                                registerPart->emulationTimingMetrics[6] = program->emulatedCycleCount;
                            registerPart->emulationTimingMetrics[4] = line;
                            registerPart->emulationTimingMetrics[5] = entryIndex;
                        }
                        registerPart->emulationTimingMetrics[7] = line;
                        registerPart->emulationTimingMetrics[8] = entryIndex;
                    }
            }
            if (reg && op->externalBlockModeFlag) {
                for (int part = 0; part != reg->partCount; part++)
                    if ((op->componentMask >> part) & 1) {
                        rPart *rp = &reg->parts[part];
                        int lat;
                        if (rp->pendingSource && (lat = rp->pendingSource->cycleOffset) != 0) {
                            ol_s tmp;
                            tmp.init(in, o, part, false);
                            if (commit && !program->proc.stalls.addStall(rp->pendingSource, &tmp, 0, lat)) {
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
                    if (reg->tracksHotOutputPaths) {
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
                                if (!program->proc.stalls.addStall(rp->pendingSource, &tmp, 1, issueLock)) {
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
                if (!Cycle(program, uta)) {
                    reportError("failed to cycle cpu\n");
                    exitAfterCleanup(-1);
                }
            } while (--stall);
        }
        cyc = program->emulatedCycleCount;
    }
    for (entryIndex = 0; entryIndex != 2; entryIndex++)
        if (row->entries[entryIndex].instruction)
            row->entries[entryIndex].instruction->scheduledCycle = cyc;

    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        in = row->entries[entryIndex].instruction;
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
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        Instr *in = row->entries[entryIndex].instruction;
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
                        if (op->externalBlockModeFlag) {
                            if (!rp->flushPending()) {
                                reportError("failed to flush pending outputs before a blocking output\n");
                                return -1;
                            }
                            ol_s *location = &rp->currentLocation;
                            location->init(in, o, part, false);
                            location->cycleOffset = clampByte(op->outputLatency);
                            rp->pendingSource = location;
                            rp->cycleAge = 0;
                        } else {
                            ol_s *ol = rp->acquireFreePendingLocation();
                            if (!ol) {
                                reportError("EMU: failed to get OL\n");
                                return -1;
                            }
                            ol->init(in, o, part, false);
                            ol->cycleOffset = clampByte(op->outputLatency);
                            rp->pendingSource = ol;
                        }
                        rp->emulationTimingMetrics[1] += op->outputLatency;
                        if (op->outputLatency > rp->emulationTimingMetrics[3])
                            rp->emulationTimingMetrics[3] = op->outputLatency;
                        int tp = 1;
                        if (reg->tracksHotOutputPaths)
                            tp = in->baseIssueSpacing;
                        if (tp > rp->emulationTimingMetrics[2])
                            rp->emulationTimingMetrics[2] = tp;
                        if (rp->emulationTimingMetrics[9] < 0) {
                            rp->emulationTimingMetrics[9] = line;
                            rp->emulationTimingMetrics[10] = entryIndex;
                            if (uta)
                                rp->emulationTimingMetrics[11] = -1;
                            else
                                rp->emulationTimingMetrics[11] = program->emulatedCycleCount;
                        }
                        rp->emulationTimingMetrics[12] = line;
                        rp->emulationTimingMetrics[13] = entryIndex;
                        if (uta)
                            rp->emulationTimingMetrics[14] = -1;
                        else
                            rp->emulationTimingMetrics[14] = program->emulatedCycleCount;
                        rp->emulationTimingMetrics[16] = op->outputLatency;
                        rp->emulationTimingMetrics[15] = tp;
                        rp->emulationTimingMetrics[0] += tp;
                        rp->issueLock = tp;
                    }
                }
            }
        }
    }
    stalls++;
    if (!Cycle(program, uta)) {
        reportError("failed to cycle cpu!\n");
        return -1;
    }
    return stalls;
}
