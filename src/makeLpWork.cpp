#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

extern const char lpWorkSavedTableError[];
extern const char lpWorkHtNewError[];
extern const char lpWorkCloneTableError[];
extern const char lpWorkNoTableError[];
extern const char lpWorkDisposeNoTableError[];
extern const char lpWorkDisposeNoSavedError[];
extern const char lsqChainHopeError[];
extern const char lsqChainLinkError[];
extern const char lsqChainNotInputError[];
extern const char lsqChainAddError[];
extern const char lsqChainNewInstrError[];


bool makeLpWork(instStream *s, int maxSpread)
{
    workTable *wt = s->workTablePtr;
    if (!wt) {
        reportError(lpWorkNoTableError);
        return false;
    }
    if (s->savedTable) {
        reportError(lpWorkSavedTableError);
        return false;
    }
    s->savedTable = wt;
    s->workTablePtr = 0;
    workTable *nt = wt->localClone();
    s->workTablePtr = nt;
    if (!nt) {
        reportError(lpWorkCloneTableError);
        return false;
    }
    Instr *in;
    for (in = nt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        int latest = 0;
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int j = 0; j != reg->partCount; j++) {
                    ol_s *ol = op->parts[j].operandLocations.head;
                    if (ol) {
                        Instr *src = ol->instruction;
                        Operand *o = operandAtLocation(ol);
                        if (!src || !o) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int v = src->instructionId + o->outputLatency + op->additionalLatency;
                        if (latest < v)
                            latest = v;
                    }
                }
            }
        }
        in->instructionId = latest;
    }
    for (in = nt->linearInstructions.tail; in; in = in->linearLink.previousInstruction) {
        int earliest = 30000;
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 2);
            if (reg) {
                for (int j = 0; j != reg->partCount; j++) {
                    for (ol_s *ol = op->parts[j].operandLocations.head; ol; ol = ol->next) {
                        Instr *dst = ol->instruction;
                        Operand *o = operandAtLocation(ol);
                        if (!dst || !o) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int v = dst->instructionId - op->outputLatency - o->additionalLatency;
                        if (earliest > v)
                            earliest = v;
                    }
                }
            }
        }
        if (earliest != 30000) {
            if (earliest < in->instructionId) {
                reportError(lpWorkHtNewError);
                return false;
            }
            in->instructionId = earliest;
        }
    }
    char buf[256];
    for (in = nt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        int lo = 30000;
        int hi = 0;
        char *p = buf;
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                p += formatIntoBuffer(p, " %s: ", reg->registerName());
                int rlo = 30000, rhi = 0;
                bool any = false;
                for (int j = 0; j != reg->partCount; j++) {
                    if ((op->componentMask >> j) & 1) {
                        ol_s *ol = op->parts[j].operandLocations.head;
                        if (ol) {
                            Instr *src = ol->instruction;
                            int d = in->instructionId - src->instructionId;
                            any = true;
                            if (d < rlo)
                                rlo = d;
                            if (d > rhi)
                                rhi = d;
                            p += formatIntoBuffer(p, "%3d ", d);
                        } else
                            p += formatIntoBuffer(p, "xxx ");
                    } else
                        p += formatIntoBuffer(p, "... ");
                }
                if (any) {
                    p += formatIntoBuffer(p, "[%d %d] (%d) ", rlo, rhi, rhi - rlo);
                    if (rhi - rlo > maxSpread)
                        p += formatIntoBuffer(p, "!!!");
                    if (rlo < lo)
                        lo = rlo;
                    if (rhi > hi)
                        hi = rhi;
                }
            }
        }
        p += formatIntoBuffer(p, "    -> %d %d (%d)", lo, hi, hi - lo);
        if (lo > maxSpread)
            p += formatIntoBuffer(p, " ****");
        p[10] = 0;
    }
    return true;
}

bool disposeLpWork(instStream *s)
{
    if (s->workTablePtr == 0) {
        reportError(lpWorkDisposeNoTableError);
        return false;
    }
    delete s->workTablePtr;
    if (s->savedTable == 0) {
        reportError(lpWorkDisposeNoSavedError);
        return false;
    }
    s->workTablePtr = s->savedTable;
    s->savedTable = 0;
    return true;
}

bool keepLpWork(instStream *s)
{
    s->savedTable = 0;
    return true;
}

bool modLsqq(Instr *in, int imm);

bool lsqChain(workTable *wt)
{
    reportError(lsqChainHopeError);
    abortAfterError();
    for (regUse *ru = wt->registerUses.head; ru; ru = ru->next) {
        RegPoolEntry *reg = ru->registerEntry;
        if (reg->registerClass != 8)
            continue;
        Instr *list[1024];
        Instr *in = wt->linearInstructions.head;
        reportInfo("lsqi- scan %s\n", reg->registerName());
        int n = 0;
        bool bad = false;
        bool ok = true;
        for (; in; in = in->linearLink.nextInstruction) {
            int opc = in->opcodeId();
            if (in->outputMaskForRegister(reg, 0xff)) {
                if (!(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4) || bad) {
                    ok = false;
                    break;
                }
                list[n++] = in;
            }
            if (in->inputMaskForRegister(reg, 0xff)
                && !(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4))
                bad = true;
        }
        if (!ok || n <= 1)
            continue;
        reportInfo("lsChain for %s\n", reg->registerName());
        Operand *src = &list[0]->operands[1];
        if (src->externalPartMask != 1) {
            reportError("this isn't externally set!\n");
            abortAfterError();
        }
        int total = 0;
        for (int i = 0; i != n; i++) {
            Instr *in = list[i];
            int opc = in->opcodeId();
            int delta = 0;
            switch (opc) {
            case 0x9e:
            case 0xa3:
                delta = 1;
                break;
            case 0x9f:
            case 0xa4:
                delta = -1;
                break;
            }
            if (!modLsqq(in, total))
                reportError("mod instruction\n");
            total += delta;
            const char *name = "NO_BASE_INST";
            if (in->opcodeInfo)
                name = in->opcodeInfo->mnemonic;
            reportInfo("  %3d : %s\n", i, name);
        }
        iInfo *ii;
        if (total < 0) {
            ii = getOpcodeInfoById(0x70);
            total = -total;
        } else
            ii = getOpcodeInfoById(0x6e);
        Instr *ni = new Instr(ii);
        if (!ni) {
            reportError(lsqChainNewInstrError);
            return false;
        }
        ni->operands[0].assignedRegister = reg;
        ni->operands[0].externalPartMask = 1;
        ni->operands[0].componentMask = 1;
        ni->operands[1].assignedRegister = reg;
        ni->operands[1].externalPartMask = 1;
        ni->operands[1].componentMask = 1;
        ni->operands[2].immediateValue.setInt(total);
        if (!wt->logAddLinear_After(list[n - 1], ni)) {
            reportError(lsqChainAddError);
            return false;
        }
        for (Instr *in = ni->linearLink.nextInstruction; in; in = in->linearLink.nextInstruction) {
            for (int k = 0; k != in->operandCount; k++) {
                Operand *op = &in->operands[k];
                if (op->assignedRegister != reg)
                    continue;
                if (!isRegisterOperand(op, 1)) {
                    reportError(lsqChainNotInputError);
                    return false;
                }
                op->parts[0].operandLocations.clear();
                if (!link_oParts(ni, 0, 0, in, k, 0)) {
                    reportError(lsqChainLinkError);
                    return false;
                }
            }
        }
    }
    return true;
}

bool splitStreams()
{
    reportError("split streams called\n");
    return false;
}

bool modLsqq(Instr *in, int imm)
{
    iInfo *ii;
    switch ((unsigned char)in->ramAccessType) {
    case 1:
        ii = getOpcodeInfoById(0x9b);
        break;
    case 2:
        ii = getOpcodeInfoById(0xa0);
        break;
    default:
        reportError("mod_lsqq: ?\n");
        abortAfterError();
        return false;
    }
    in->opcodeInfo = ii;
    in->operands[2].parts[0].clearSlotLocations();
    in->operands[1].parts[0].clearSlotLocations();
    in->operandCount = 2;
    in->operands[1].externalPartMask = 1;
    Operand *op = &in->operands[1];
    iOpDesc *d = &ii->operandDescriptors[1];
    op->operandKind = d->operandKind;
    op->isImplicit = d->isImplicit != 0;
    op->unk_05 = d->unk_05;
    op->pairedOperandIndexPlusOne = d->patchedOutputIndexPlusOne;
    if (op->operandKind != 0x1d) {
        reportError("modded lqsq isn't ea_imm_reg\n");
        abortAfterError();
        return false;
    }
    if (!op->immediateValue.setInt(imm)) {
        reportError("failed to set new immediate\n");
        return false;
    }
    return true;
}
