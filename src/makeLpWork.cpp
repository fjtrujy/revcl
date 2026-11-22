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
    if (s->unk_220) {
        reportError(lpWorkSavedTableError);
        return false;
    }
    s->unk_220 = wt;
    s->workTablePtr = 0;
    workTable *nt = wt->localClone();
    s->workTablePtr = nt;
    if (!nt) {
        reportError(lpWorkCloneTableError);
        return false;
    }
    Instr *in;
    for (in = nt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int latest = 0;
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int j = 0; j != reg->partCount; j++) {
                    ol_s *ol = op->unk_2c[j].unk_54.unk_00;
                    if (ol) {
                        Instr *src = ol->unk_04;
                        Operand *o = func_0805802c(ol);
                        if (!src || !o) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int v = src->unk_0c + o->unk_0a + op->unk_29;
                        if (latest < v)
                            latest = v;
                    }
                }
            }
        }
        in->unk_0c = latest;
    }
    for (in = nt->unk_88.unk_04; in; in = in->unk_1c.unk_04) {
        int earliest = 30000;
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 2);
            if (reg) {
                for (int j = 0; j != reg->partCount; j++) {
                    for (ol_s *ol = op->unk_2c[j].unk_54.unk_00; ol; ol = ol->unk_00) {
                        Instr *dst = ol->unk_04;
                        Operand *o = func_0805802c(ol);
                        if (!dst || !o) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int v = dst->unk_0c - op->unk_0a - o->unk_29;
                        if (earliest > v)
                            earliest = v;
                    }
                }
            }
        }
        if (earliest != 30000) {
            if (earliest < in->unk_0c) {
                reportError(lpWorkHtNewError);
                return false;
            }
            in->unk_0c = earliest;
        }
    }
    char buf[256];
    for (in = nt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
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
                    if ((op->unk_07 >> j) & 1) {
                        ol_s *ol = op->unk_2c[j].unk_54.unk_00;
                        if (ol) {
                            Instr *src = ol->unk_04;
                            int d = in->unk_0c - src->unk_0c;
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
    if (s->unk_220 == 0) {
        reportError(lpWorkDisposeNoSavedError);
        return false;
    }
    s->workTablePtr = s->unk_220;
    s->unk_220 = 0;
    return true;
}

bool keepLpWork(instStream *s)
{
    s->unk_220 = 0;
    return true;
}

bool modLsqq(Instr *in, int imm);

bool lsqChain(workTable *wt)
{
    reportError(lsqChainHopeError);
    abortAfterError();
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (reg->registerClass != 8)
            continue;
        Instr *list[1024];
        Instr *in = wt->unk_88.unk_00;
        reportInfo("lsqi- scan %s\n", reg->registerName());
        int n = 0;
        bool bad = false;
        bool ok = true;
        for (; in; in = in->unk_1c.unk_00) {
            int opc = in->func_08055ac4();
            if (in->func_080553b4(reg, 0xff)) {
                if (!(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4) || bad) {
                    ok = false;
                    break;
                }
                list[n++] = in;
            }
            if (in->func_0805544c(reg, 0xff)
                && !(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4))
                bad = true;
        }
        if (!ok || n <= 1)
            continue;
        reportInfo("lsChain for %s\n", reg->registerName());
        Operand *src = &list[0]->operands[1];
        if (src->unk_27 != 1) {
            reportError("this isn't externally set!\n");
            abortAfterError();
        }
        int total = 0;
        for (int i = 0; i != n; i++) {
            Instr *in = list[i];
            int opc = in->func_08055ac4();
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
                name = in->opcodeInfo->unk_04;
            reportInfo("  %3d : %s\n", i, name);
        }
        iInfo *ii;
        if (total < 0) {
            ii = func_0806bf74(0x70);
            total = -total;
        } else
            ii = func_0806bf74(0x6e);
        Instr *ni = new Instr(ii);
        if (!ni) {
            reportError(lsqChainNewInstrError);
            return false;
        }
        ni->operands[0].unk_18 = reg;
        ni->operands[0].unk_27 = 1;
        ni->operands[0].unk_07 = 1;
        ni->operands[1].unk_18 = reg;
        ni->operands[1].unk_27 = 1;
        ni->operands[1].unk_07 = 1;
        ni->operands[2].unk_1c.setInt(total);
        if (!wt->logAddLinear_After(list[n - 1], ni)) {
            reportError(lsqChainAddError);
            return false;
        }
        for (Instr *in = ni->unk_1c.unk_00; in; in = in->unk_1c.unk_00) {
            for (int k = 0; k != in->operandCount; k++) {
                Operand *op = &in->operands[k];
                if (op->unk_18 != reg)
                    continue;
                if (!isRegisterOperand(op, 1)) {
                    reportError(lsqChainNotInputError);
                    return false;
                }
                op->unk_2c[0].unk_54.clear();
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
    switch ((unsigned char)in->unk_13ce) {
    case 1:
        ii = func_0806bf74(0x9b);
        break;
    case 2:
        ii = func_0806bf74(0xa0);
        break;
    default:
        reportError("mod_lsqq: ?\n");
        abortAfterError();
        return false;
    }
    in->opcodeInfo = ii;
    in->operands[2].unk_2c[0].func_080560e8();
    in->operands[1].unk_2c[0].func_080560e8();
    in->operandCount = 2;
    in->operands[1].unk_27 = 1;
    Operand *op = &in->operands[1];
    iOpDesc *d = &ii->unk_44[1];
    op->unk_04 = d->unk_00;
    op->unk_08 = d->unk_02 != 0;
    op->unk_05 = d->unk_05;
    op->unk_0d = d->unk_06;
    if (op->unk_04 != 0x1d) {
        reportError("modded lqsq isn't ea_imm_reg\n");
        abortAfterError();
        return false;
    }
    if (!op->unk_1c.setInt(imm)) {
        reportError("failed to set new immediate\n");
        return false;
    }
    return true;
}
