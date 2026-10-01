#pragma implementation "fDep.h"
#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"
#include "fDep.h"

bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool func_08065fd4(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

Operand *getEA(Instr *i)
{
    iInfo *ii = i->unk_34;
    if (!ii) {
        func_0804ad38("no ii for getEA\n");
        func_0804adf0(-1);
    }
    if ((ii->unk_18 & 7) == 4) {
        if (ii->unk_1a >= i->unk_138b) {
            func_0804ad38("eaop is out of bounds for operands st=%d\n", ii->unk_19);
            i->leDump();
            func_0804adf0(-1);
        }
        Operand *op = &i->unk_38[ii->unk_1a];
        if (op->unk_09 != 1) {
            func_0804ad38("effective address specifier isn't an input\n");
            func_0804adf0(-1);
        }
        return op;
    }
    return 0;
}

bool sameEA(Instr *a, Instr *b)
{
    if (!a || !b || !a->unk_13ce || !b->unk_13ce || a->unk_30 != b->unk_30)
        return false;
    Operand *ea1 = getEA(a);
    Operand *ea2 = getEA(b);
    if (!ea1 || !ea2) {
        func_0804ad38("failed to get effective address operand\n");
        func_0804adf0(-1);
    }
    if (ea1->unk_27 != ea2->unk_27)
        return false;
    RegPoolEntry *r1 = ea1->unk_18;
    RegPoolEntry *r2 = ea2->unk_18;
    if (r1 && r1 == r1->unk_04->unk_10)
        r1 = 0;
    if (r2 && r2 == r2->unk_04->unk_10)
        r2 = 0;
    if (r1 != r2)
        return false;
    if (r1) {
        if (ea1->unk_07 != ea2->unk_07)
            return false;
        if (ea1->unk_07 != 1) {
            func_0804ad38("mod bits not 1 for an effective address specifier\n");
            func_0804adf0(-1);
        }
        if (!func_080585d4(ea1->unk_2c[0].unk_54.unk_00, ea2->unk_2c[0].unk_54.unk_00))
            return false;
    }
    if (func_0804e5c0(&ea1->unk_1c, &ea2->unk_1c))
        return true;
    return false;
}

bool func_0806558c(Instr *i, int *row)
{
    *row = 0;
    for (ol_s *o = i->unk_13a0.unk_00; o; o = o->unk_00) {
        Instr *in = o->unk_04;
        if (!in) {
            func_0804ad38("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->unk_13e8.unk_00;
        if (r < 0) {
            *row = -1;
            return true;
        }
        if (r + 1 > *row)
            *row = r + 1;
    }
    return true;
}

bool func_080655e8(Instr *i, int *row)
{
    *row = 30000;
    for (ol_s *o = i->unk_13a8.unk_00; o; o = o->unk_00) {
        Instr *in = o->unk_04;
        if (!in) {
            func_0804ad38("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->unk_13e8.unk_00;
        if (r >= 0 && r < *row)
            *row = r;
    }
    return true;
}

bool instStream::func_0806563c(bool a, bool b)
{
    if (!unk_218) {
        func_0804ac7c("BuildfDepList - no work table set for %s!\n", func_0806d674());
        return false;
    }
    return BuildfDepList(unk_218, a, b);
}

bool BuildfDepList(workTable *wt, bool force, bool)
{
    Instr *in;
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        in->unk_1410 = 0;
        in->unk_1412 = 0;
        in->unk_1414 = 0;
        if (!in->unk_13a0.clear()) {
            func_0804ad38("failed to dispose of existing prefdeplist\n");
            return false;
        }
        if (!in->unk_13a8.clear()) {
            func_0804ad38("failed to dispose of existing postfdeplist\n");
            return false;
        }
    }
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        bool b40 = false;
        bool b10 = false;
        int ramBits = 0;
        Operand *ea = 0;
        bool b20 = false;
        if (in->unk_13d0 & 0x08) {
            func_0804ad38("found an instruction barrier fdep - this should be done with --barrier fixup!\n");
            in->leDump();
            return false;
        }
        if (in->unk_13d0 & 0x10)
            b10 = true;
        if (in->unk_13d0 & 0x20)
            b20 = true;
        if (in->unk_13d0 & 0x40)
            b40 = true;
        if (in->unk_13ce == 2) {
            ea = getEA(in);
            if (!ea) {
                func_0804ad38("failed to get ea op for store\n");
                return false;
            }
            b10 = true;
            b20 = true;
            ramBits = in->getRamBits();
            if (!ramBits)
                func_0804ac7c("found store instruction with no ram bits\n");
        }
        if (in->unk_13ce == 1) {
            ea = getEA(in);
            if (!ea) {
                func_0804ad38("failed to get ea op for load\n");
                return false;
            }
            b20 = true;
            ramBits = in->getRamBits();
            if (!ramBits)
                func_0804ac7c("found load instruction with no ram bits\n");
        }
        if (b20 || b10 || b40) {
            for (Instr *o = in->unk_1c.unk_04; o; o = o->unk_1c.unk_04) {
                bool dep = false;
                int type = (unsigned char)o->unk_13ce;
                if (((b20 && type == 2) || (b10 && type == 1))
                    && (!ea || ((o->getRamBits() & ramBits)
                                && (force || tagsMatch(o, in) || sameEA(in, o)))))
                    dep = true;
                else if (b40 && (o->unk_13d0 & 0x40)) {
                    b40 = false;
                    dep = true;
                } else {
                    if (in->unk_13ce == 2 && (o->unk_13d0 & 0x20))
                        dep = true;
                    if (in->unk_13ce == 1 && (o->unk_13d0 & 0x10))
                        dep = true;
                }
                if (dep) {
                    if (!fdep_Link(o, in)) {
                        func_0804ad38("failed to add ol for fdeps \n");
                        func_0804adf0(-1);
                    }
                }
            }
        } else if (in->unk_139c) {
            func_0804ad38("tag string, but no match criteria set\n");
            return false;
        }
        for (int k = 0; k != in->unk_138b; k++) {
            Operand *op = &in->unk_38[k];
            if (op) {
                RegPoolEntry *r = op->getReg2();
                if (r) {
                    if (r->unk_30 == 48)
                        in->unk_1414 = op->unk_07;
                    if (r->unk_30 == 48 || r->unk_30 == 49) {
                        for (int p = 0; p != r->unk_28; p++)
                            if ((op->unk_07 >> p) & 1)
                                for (ol_s *l = op->unk_2c[p].unk_54.unk_00; l; l = l->unk_00) {
                                    Instr *x = l->unk_04;
                                    if (x)
                                        func_08065fd4(in, x);
                                }
                    }
                }
                RegPoolEntry *w = op->inputReg();
                if (w && w->unk_30 == 48)
                    in->unk_1412 = op->unk_07;
            }
        }
    }
    in = wt->unk_88.unk_00;
    Instr *last[4] = { 0, 0, 0, 0 };
    for (; in; in = in->unk_1c.unk_00) {
        int p;
        for (p = 0; p != 4; p++)
            if (((in->unk_1414 >> p) & 1) && !((in->unk_1412 >> p) & 1))
                last[p] = in;
        for (p = 0; p != 4; p++) {
            if ((in->unk_1412 >> p) & 1) {
                Instr *w = last[p];
                if (w)
                    for (ol_s *l = in->unk_13a0.unk_00; l; l = l->unk_00) {
                        Instr *pre = l->unk_04;
                        if (pre->unk_13e4 < w->unk_13e4) {
                            w->unk_1410 = 1;
                            w->unk_13a0.func_0805826c(pre);
                        }
                    }
            }
        }
        in->func_08054008(false);
        in->unk_13a0.func_08057e9c();
        in->unk_13a8.func_08057e9c();
    }
    return true;
}

bool tagsMatch(Instr *a, Instr *b)
{
    char *p = a->unk_139c;
    if (!p)
        return false;
    char *q0 = b->unk_139c;
    if (!q0)
        return false;
    while (*p) {
        char *q = q0;
        while (*q) {
            char *s = p;
            while (*q && *s && *q != ':' && *s != ':') {
                q++;
                s++;
            }
            if (*q == *s)
                return true;
            while (*q && *q != ':')
                q++;
            if (*q == ':')
                q++;
        }
        while (*p && *p != ':')
            p++;
        if (*p == ':')
            p++;
    }
    return false;
}

bool fixupCloneFdeps(workTable *wt)
{
    int n = 0;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        Instr *orig = in->unk_08;
        n += 8;
        in->unk_13e4 = n;
        if (!orig) {
            func_0804ad38("no original set\n");
            func_0804adf0(-1);
        }
        in->unk_1410 = orig->unk_1410;
        if (!copyCloneFdeps(in, orig->unk_13a0.unk_00, &in->unk_13a0)) {
            func_0804ad38("failed to fixup the pre fdep list\n");
            return false;
        }
        if (!copyCloneFdeps(in, orig->unk_13a8.unk_00, &in->unk_13a8)) {
            func_0804ad38("failed to fix the post fdeps\n");
            return false;
        }
    }
    return true;
}
