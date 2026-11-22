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
        reportWarning("emu - passed a null table ptr!\n");
        return true;
    }
    if (wt->getLoopSize() || wt->mainLoopId) {
        reportError("mlid is set, or loop size is set for emu..which is now kinda tricky..\n");
        return false;
    }
    wt->declaredRawLength = -1;
    switch (wt->codeModeValue) {
    case 3: {
        int skipped = 0;
        for (int line = wt->getRangeStart(); line != wt->getRangeEnd(); line++) {
            wtRow *r = wt->getRowByIndex(line);
            if (r) {
                if (r->skip) {
                    for (int k = 0; k != 2; k++)
                        if (r->entries[k].instruction) {
                            reportError("m_bSkip set .. but line contains instructions\n");
                            return false;
                        }
                    skipped++;
                    continue;
                }
                if (emuInst(p, r, line, false, true) < 0) {
                    reportError("failed to Emu an inst\n");
                    r->refreshInstructionOperands(-1);
                    goto fail;
                }
            } else {
                reportError("emuuta - failed to get an ie\n");
                return false;
            }
        }
        wt->declaredRawLength = p->emulatedCycleCount;
        skipped += p->emulatedCycleCount;
        if (skipped < wt->getPhysicalLen()) {
            reportError("logical len < physical len.. %d<%d!\n", p->emulatedCycleCount, wt->getPhysicalLen());
            wt->dump();
            return false;
        }
        break;
    }
fail:
    return false;
    case 1:
    case 4: {
        Instr *i = wt->linearInstructions.head;
        wtRow row;
        for (; i; i = i->linearLink.nextInstruction) {
            row.clearEntries();
            row.entries[0].instruction = i;
            int r = emuInst(p, &row, -1, true, true);
            row.entries[0].instruction = 0;
            if (r < 0) {
                reportError("failed to uta emu instruction\n");
                i->dumpOperandDiagnostics(false);
                goto fail;
            }
        }
        break;
    }
    default:
        const char *modeName = codeModeName(wt->codeModeValue);
        reportError("emu - can't emu table from %s of type %s\n",
                      wt->ownerName(), modeName);
        break;
    }
    return true;
}

static inline void reportRegRefFailure()
{
    reportError("failed to add reg ref\n");
}

bool emulateAndCollectRegisterUses(Program *p, instStream *s)
{
    int count;
    Instr *i;
    workTable *wt;

    wt = s->getTable();
    int linear = wt->instructionCountsByPipe[2];
    int iCount = wt->linearInstructions.count;
    if (linear != iCount) {
        reportError("linear count not same as iCount %d!=%d\n", linear, iCount);
        return false;
    }
    if (!emuTable(p, wt)) {
        reportError("Processor : failed to emu itable from '%s'\n", s->streamName());
        return false;
    }
    count = 0;
    for (i = wt->linearInstructions.head; i; i = i->linearLink.nextInstruction) {
        count++;
        for (int op = 0; op != i->operandCount; op++) {
            Operand *o = &i->operands[op];
            RegPoolEntry *reg = o->assignedRegister;
            if (!reg)
                continue;
            regUse *ru = wt->registerUses.findByRegister(reg);
            if (!ru) {
                ru = wt->registerUses.AddNewRegRef(reg);
                if (!ru) {
                    reportRegRefFailure();
                    return false;
                }
            }
            if (isRegisterOperand(o, 2))
                ru->outputMask |= o->componentMask;
            if (isRegisterOperand(o, 1))
                ru->inputMask |= o->componentMask;
            if (isRegisterOperand(o, 2)) {
                if (o->externalPartMask) {
                    reportError("operand had xbits set for output!\n");
                    return false;
                }
            }
            ru->externalInputMask |= o->externalPartMask;
            for (int k = 0; k != reg->partCount; k++) {
                if ((o->externalPartMask >> k) & 1) {
                    if (ru->externalBlockMode == -1)
                        ru->externalBlockMode = o->unk_0b;
                    else if (ru->externalBlockMode != (unsigned char)o->unk_0b)
                        reportWarning("differing block modes for external input for %s..taking first encountered\n",
                                      reg->registerName());
                    if (!ru->slots[k].inputLocations.addOperandLocation(i, op, k, false)) {
                        reportError("failed to add oploc user!\n");
                        return false;
                    }
                }
                if (((o->componentMask >> k) & 1) && reg->unk_2c && isRegisterOperand(o, 2)) {
                    if (!ru->slots[k].sourceLocations.addOperandLocation(i, op, k, false)) {
                        reportError("failed to add a local setter for a thruput check\n");
                        reportError("register is %s\n", reg->registerName());
                        return false;
                    }
                }
            }
        }
    }
    if (count != wt->instructionCountsByPipe[2]) {
        reportError("icount mismatch (%d apparently in list, but %d in table?)\n", count, wt->instructionCountsByPipe[2]);
        return false;
    }
    for (regUse *ru = wt->registerUses.head; ru; ru = ru->next) {
        RegPoolEntry *reg = ru->registerEntry;
        if (!reg) {
            reportError("a reg ref had no register!\n");
            return false;
        }
        for (int part = 0; part != reg->partCount; part++) {
            regUseSlot *slot = &ru->slots[part];
            rPart *rp = &reg->parts[part];
            slot->timingValues[0] = rp->unk_00[4];
            slot->timingValues[1] = rp->unk_00[5];
            slot->timingValues[2] = rp->unk_00[6];
            slot->timingValues[3] = rp->unk_00[7];
            slot->timingValues[4] = rp->unk_00[8];
            slot->timingValues[5] = rp->unk_00[9];
            slot->timingValues[6] = rp->unk_00[10];
            slot->timingValues[7] = rp->unk_00[11];
            slot->timingValues[8] = rp->unk_00[12];
            slot->timingValues[9] = rp->unk_00[13];
            slot->timingValues[10] = rp->unk_00[14];
            slot->timingValues[11] = rp->unk_00[15];
            slot->timingValues[12] = rp->unk_00[16];
            slot->throughputSums[1] = rp->unk_00[1];
            slot->throughputSums[0] = rp->unk_00[0];
            slot->throughputSums[3] = rp->unk_00[2];
            slot->throughputSums[2] = rp->unk_00[3];
            if ((ru->outputMask >> part) & 1) {
                ol_s *out = &rp->currentLocation;
                if (out->instruction && !slot->addOutput(out)) {
                    reportError("failed to add an output to an rrp\n");
                    return false;
                }
                for (int j = 0; j != 4; j++) {
                    ol_s *ol = rp->getPendingLocation(j);
                    if (!ol) {
                        reportError("failed to get a pool entry\n");
                        return false;
                    }
                    if (ol->cycleOffset && !slot->addOutput(ol)) {
                        reportError("failed to add a pending output to an rrp\n");
                        return false;
                    }
                }
                if (slot->outputLocations.count <= 0) {
                    reportError("no or -ve outputs during out build..nOut=%d!\n", slot->outputLocations.count);
                    return false;
                }
            }
        }
    }
    return true;
}
