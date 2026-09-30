#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart)
{
    workTable *wt = s->getTable();
    if (loopSize) {
        if (!s->canLoop()) {
            func_0804ad38("prepLockTable: can't loop, but loop size requested\n");
            return 0;
        }
        if (wt->unk_09 == 3) {
            func_0804ad38("prepLockTable: loopSize requested, but mode is RAW\n");
            return 0;
        }
    }
    if (!optReset(p)) {
        func_0804ad38("failed to optReset cpu\n");
        return 0;
    }
    if (!wt->clearClones()) {
        func_0804ad38("failed to flush workClones table for %s\n", s->func_0806d674());
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        func_0804ad38("iStream::prepUta - failed alloc table\n");
        func_0804adf0(-1);
    }
    if (!nt->init(2, s->unk_2bc)) {
        func_0804ad38("failed to allocate large table\n");
        delete nt;
        return 0;
    }
    nt->unk_10 = s;
    if (loopSize) {
        nt->unk_7a = s->unk_274;
        nt->setRange(loopSize);
    } else
        nt->setRange();
    setRangeFrom(nt);
    g_080de284 = smart;
    nt->unk_5c.func_0805e37c(&s->unk_278);
    nt->unk_74 = wt->unk_74;
    nt->unk_76 = wt->unk_76;
    nt->unk_78 = wt->unk_78;
    int mode = (unsigned char)wt->unk_08;
    nt->unk_08 = mode;
    nt->unk_40 = 0;
    nt->unk_44 = wt->unk_44;
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        if (!nt->qRegRef(ru)) {
            func_0804ad38("failed to add a cloned regref in %s\n", s->func_0806d674());
            wt->dump();
            delete nt;
            return 0;
        }
    }
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (in->func_08054168() != s) {
            func_0804ad38("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          in->func_08054168(), p);
            in->leDump();
            func_0804adf0(-1);
        }
        in->unk_141a = 0;
        if (in->func_080541fc() && !in->func_08054138() && !nt->qInstruction(in)) {
            func_0804ad38("iStream: failed to queue instruction into table for opt\n");
            func_0804adf0(-1);
        }
    }
    if (!fixupCloneFdeps(nt)) {
        func_0804ad38("failed to fix fdeps to new table\n");
        return 0;
    }
    return nt;
}

workTable *makeForceTable(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        func_0804ad38("it_old is not raw mode - %s\n", codeModeName(mode));
        return 0;
    }
    workTable *nt = prepUta(p, s, 0, 0);
    if (!nt) {
        func_0804ad38("failed to make a lock table for makeForceTable\n");
        return 0;
    }
    mode = nt->unk_09;
    if ((unsigned char)mode != 2) {
        func_0804ad38("it_new is not raw mode .. %s\n", codeModeName(mode));
        return 0;
    }
    int start = wt->func_0807411c();
    int end = wt->func_080741c0();
    nt->setRange(start, end);
    for (Instr *in = nt->unk_94.unk_00; in; in = nt->unk_94.unk_00) {
        Instr *clone = in->unk_08;
        cls08058718 pos;
        cls08058718 *r = &clone->unk_13e8;
        pos.unk_00 = r->unk_00;
        pos.unk_04 = r->unk_04;
        pos.unk_03 = r->unk_03;
        pos.unk_02 = r->unk_02;
        if (!nt->forceInsert(in, &pos)) {
            func_0804ad38("failed to force an insert into table\n");
            delete nt;
            return 0;
        }
    }
    nt->changeMode(3);
    nt->func_08073774(wt->func_08073754());
    return nt;
}

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
    RegPoolEntry *reg;
    for (in = nt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int latest = 0;
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            reg = op->getReg();
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
            RegPoolEntry *reg = op->getReg2();
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
            reg = op->getReg();
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
                            p += func_0804adbc(p, "%3d ", d, src);
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
