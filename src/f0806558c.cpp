#include "fDep_shared.h"

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
    in = wt->unk_88.unk_00;
    if (!in)
        goto finalize;
    for (; in; in = in->unk_1c.unk_00) {
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
    {
        int haveInstruction = (in = (Instr *)*(void **)&wt->unk_88.unk_00) != 0;
        while (haveInstruction) {
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
                    RegPoolEntry *r = registerOperand(op, 2);
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
                    r = registerOperand(op, 1);
                    if (r && r->unk_30 == 48)
                        in->unk_1412 = op->unk_07;
                }
            }
            in = in->unk_1c.unk_00;
            haveInstruction = in != 0;
        }
    }
finalize:
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
