#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);


bool makeLpWork(instStream *s, int maxSpread)
{
    workTable *wt = s->unk_218;
    if (!wt) {
        func_0804ad38("no work table set for make_lpWork \n");
        return false;
    }
    if (s->unk_220) {
        func_0804ad38("there's already a saved work table\n");
        return false;
    }
    s->unk_220 = wt;
    s->unk_218 = 0;
    workTable *nt = wt->localClone();
    s->unk_218 = nt;
    if (!nt) {
        func_0804ad38("failed to locally clone table\n");
        return false;
    }
    Instr *in;
    for (in = nt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int latest = 0;
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int j = 0; j != reg->unk_28; j++) {
                    ol_s *ol = op->unk_2c[j].unk_54.unk_00;
                    if (ol) {
                        Instr *src = ol->unk_04;
                        Operand *o = func_0805802c(ol);
                        if (!src || !o) {
                            func_0804ad38("in_s not set\n");
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
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = registerOperand(op, 2);
            if (reg) {
                for (int j = 0; j != reg->unk_28; j++) {
                    for (ol_s *ol = op->unk_2c[j].unk_54.unk_00; ol; ol = ol->unk_00) {
                        Instr *dst = ol->unk_04;
                        Operand *o = func_0805802c(ol);
                        if (!dst || !o) {
                            func_0804ad38("in_s not set\n");
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
                func_0804ad38("can't have htnew before minip\n");
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
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                p += func_0804adbc(p, " %s: ", reg->func_0805a8e0());
                int rlo = 30000, rhi = 0;
                bool any = false;
                for (int j = 0; j != reg->unk_28; j++) {
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
                            p += func_0804adbc(p, "%3d ", d);
                        } else
                            p += func_0804adbc(p, "xxx ");
                    } else
                        p += func_0804adbc(p, "... ");
                }
                if (any) {
                    p += func_0804adbc(p, "[%d %d] (%d) ", rlo, rhi, rhi - rlo);
                    if (rhi - rlo > maxSpread)
                        p += func_0804adbc(p, "!!!");
                    if (rlo < lo)
                        lo = rlo;
                    if (rhi > hi)
                        hi = rhi;
                }
            }
        }
        p += func_0804adbc(p, "    -> %d %d (%d)", lo, hi, hi - lo);
        if (lo > maxSpread)
            p += func_0804adbc(p, " ****");
        p[10] = 0;
    }
    return true;
}

bool disposeLpWork(instStream *s)
{
    if (s->unk_218 == 0) {
        func_0804ad38("no work table set for dispose lp table\n");
        return false;
    }
    delete s->unk_218;
    if (s->unk_220 == 0) {
        func_0804ad38("there's no saved work table for dispose\n");
        return false;
    }
    s->unk_218 = s->unk_220;
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
    func_0804ad38("well, let's hope never wind up in this function!\n");
    func_0804aeb0();
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (reg->unk_24 != 8)
            continue;
        Instr *list[1024];
        Instr *in = wt->unk_88.unk_00;
        func_0804acb0("lsqi- scan %s\n", reg->func_0805a8e0());
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
        func_0804acb0("lsChain for %s\n", reg->func_0805a8e0());
        Operand *src = &list[0]->unk_38[1];
        if (src->unk_27 != 1) {
            func_0804ad38("this isn't externally set!\n");
            func_0804aeb0();
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
                func_0804ad38("mod instruction\n");
            total += delta;
            const char *name = "NO_BASE_INST";
            if (in->unk_34)
                name = in->unk_34->unk_04;
            func_0804acb0("  %3d : %s\n", i, name);
        }
        iInfo *ii;
        if (total < 0) {
            ii = func_0806bf74(0x70);
            total = -total;
        } else
            ii = func_0806bf74(0x6e);
        Instr *ni = new Instr(ii);
        if (!ni) {
            func_0804ad38("failed to get new instruction\n");
            return false;
        }
        ni->unk_38[0].unk_18 = reg;
        ni->unk_38[0].unk_27 = 1;
        ni->unk_38[0].unk_07 = 1;
        ni->unk_38[1].unk_18 = reg;
        ni->unk_38[1].unk_27 = 1;
        ni->unk_38[1].unk_07 = 1;
        ni->unk_38[2].unk_1c.setInt(total);
        if (!wt->logAddLinear_After(list[n - 1], ni)) {
            func_0804ad38("failed to add the new instruction\n");
            return false;
        }
        for (Instr *in = ni->unk_1c.unk_00; in; in = in->unk_1c.unk_00) {
            for (int k = 0; k != in->unk_138b; k++) {
                Operand *op = &in->unk_38[k];
                if (op->unk_18 != reg)
                    continue;
                if (!isRegisterOperand(op, 1)) {
                    func_0804ad38("isn't an input, and this code is ..not taking any chances!\n");
                    return false;
                }
                op->unk_2c[0].unk_54.clear();
                if (!link_oParts(ni, 0, 0, in, k, 0)) {
                    func_0804ad38("failed to link parts together\n");
                    return false;
                }
            }
        }
    }
    return true;
}

bool splitStreams()
{
    func_0804ad38("split streams called\n");
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
        func_0804ad38("mod_lsqq: ?\n");
        func_0804aeb0();
        return false;
    }
    in->unk_34 = ii;
    in->unk_38[2].unk_2c[0].func_080560e8();
    in->unk_38[1].unk_2c[0].func_080560e8();
    in->unk_138b = 2;
    in->unk_38[1].unk_27 = 1;
    Operand *op = &in->unk_38[1];
    iOpDesc *d = &ii->unk_44[1];
    op->unk_04 = d->unk_00;
    op->unk_08 = d->unk_02 != 0;
    op->unk_05 = d->unk_05;
    op->unk_0d = d->unk_06;
    if (op->unk_04 != 0x1d) {
        func_0804ad38("modded lqsq isn't ea_imm_reg\n");
        func_0804aeb0();
        return false;
    }
    if (!op->unk_1c.setInt(imm)) {
        func_0804ad38("failed to set new immediate\n");
        return false;
    }
    return true;
}
