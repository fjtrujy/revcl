#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int func_0807aaf4(Program *p, instStream *s);
void func_0807bb84(workTable *wt);
bool func_0807af9c(Program *p, instStream *s);
int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask);
bool doCap(int line, int pipe, int mask);
RegPool *GetRP(unsigned t);
bool func_0806c0f8(Program *p);
int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
void splitCtime(workTable *wt, int l, int m);


bool func_0807aee4(Program *p)
{
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108)
        s->unk_2dc = s->func_0806d9a0() ? 0 : 1;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2dc)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            func_0804ad38("failed to prep external info for stream %s\n", s->func_0806d674());
            return false;
        }
        if (!func_0807af9c(p, s)) {
            func_0804ad38("failed to sred a table from %s\n", s->func_0806d674());
            return false;
        }
        s->unk_2dc = 1;
    }
    return true;
}

bool func_0807af9c(Program *p, instStream *s)
{
    if (!s->unk_218) {
        func_0804ad38("work table not set in %s\n", s->func_0806d674());
        func_0804adf0(-1);
    }
    workTable *wt = s->unk_218;
    if (!wt) {
        func_0804ad38("no work table \n");
        return false;
    }
    int len = wt->func_080741c0() - wt->func_0807411c();
    if (len == 0)
        return true;
    if (wt->unk_44 < 0) {
        func_0804ad38("the minimal length is -ve for ext\n");
        return false;
    }
    if (len < wt->unk_44) {
        func_0804ad38("table is lready too short (%d<%d)\n", len, wt->unk_44);
        return false;
    }
    RegPool *hw = GetRP(10);
    if (!hw) {
        func_0804ad38("failed to get hw register pool - maybe this isn't vpu?\n");
        return false;
    }
    func_0807bb84(wt);
    if (!func_0806c0f8(p)) {
        func_0804ad38("failed to reset for emu\n");
        return false;
    }
    bool lastOk = true;
    if (s->unk_2ce)
        lastOk = false;
    else if (s->unk_2cd != 1)
        lastOk = false;
    int curTick = 0;
    int logLen = wt->func_08072eec();
    ol_s *inflight[1024];
    for (int rl = wt->func_0807411c(); rl != wt->func_080741c0(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)wt)->getIE_ck(rl);
        if (row->unk_08) {
            func_0804ad38("skip already set for line\n");
            return false;
        }
        if (!row->func_08072304()) {
            int tNow = row->unk_0c;
            int n = p->proc.gatherInflight(inflight);
            if (n < 0) {
                func_0804ad38("error gathering inflight data\n");
                return false;
            }
            int m = wt->func_080741c0();
            int k;
            for (k = 0; k != n; k++) {
                ol_s *ol = inflight[k];
                Instr *in = ol->unk_04;
                Operand *op = func_0805802c(ol);
                RegPoolEntry *reg = op->unk_18;
                int part = ol->unk_11;
                int lat = ol->unk_12;
                if (reg->unk_2c) {
                    int tv = in->unk_13cf;
                    if (tv <= 6)
                        func_0804ae34("bad throught value in register\n");
                    int thr = lat;
                    if (reg->unk_30 == 0x34)
                        thr--;
                    else if (reg->unk_30 != 0x32)
                        func_0804ae34("non p-q has throughput?\n");
                    wtRow *r = row;
                    int l = rl + 1;
                    while (r++, l < wt->func_080741c0()) {
                        int t = r->unk_0c;
                        if (t > tNow + thr)
                            break;
                        if (r->func_080723ec(reg, 0xff)) {
                            if (t != tNow + thr) {
                                func_0804ad38("unexpected thruput stall detected\n");
                                return false;
                            }
                            if (l < m)
                                m = l;
                            break;
                        }
                        l++;
                    }
                }
                for (ol_s *u = op->unk_2c[part].unk_54.func_08058370(); u && !(u->unk_13 & 1); u = u->unk_00) {
                    Instr *ui = u->unk_04;
                    if (func_0805802c(u)->unk_0b && op->unk_0a > 1) {
                        int ul = ui->unk_13e8.unk_00;
                        int tu = wt->ctime(ul);
                        if (tu <= tNow) {
                            func_0804ad38("user is before now .. tUser=%d tNow = %d\n", tu, tNow);
                            return false;
                        }
                        if (tNow + lat > tu) {
                            func_0804ad38("found an unexpected stall\n");
                            return false;
                        }
                        if (tNow + lat == tu && ul < m)
                            m = ul;
                    }
                }
            }
            bool ok = true;
            if (rl == wt->func_080741c0() - 1 && !lastOk)
                ok = false;
            for (k = 0; k != n && ok; k++) {
                ol_s *ol = inflight[k];
                Operand *op = func_0805802c(ol);
                int part = ol->unk_11;
                int lat = ol->unk_12;
                ol_s *u = op->unk_2c[part].unk_54.func_08058370();
                if (u) {
                    Instr *ui = u->unk_04;
                    Operand *uo = func_0805802c(u);
                    int ul = ui->unk_13e8.unk_00;
                    int tu = wt->ctime(ul) - 1;
                    if (!op->unk_0b && !uo->unk_0b && ul < m && tNow + lat > tu)
                        ok = false;
                }
            }
            if (ok && rl < wt->func_080741c0() - 1 && rl + 1 < m && row[1].func_08072494() == 1)
                ok = false;
            if (m < wt->func_080741c0()) {
                int tm = wt->ctime(m);
                wtRow *r = row + 1;
                for (int l = rl + 1; l < m && ok; l++, r++) {
                    int base = r->unk_0c - 1;
                    for (int pp = 0; pp != 2; pp++) {
                        Instr *in = r->getEntry(pp);
                        if (!in)
                            continue;
                        Operand *o = in->unk_38;
                        for (int i = 0; i != in->unk_138b; i++, o++) {
                            RegPoolEntry *w = o->getReg2();
                            if (o->unk_28)
                                ok = false;
                            if (w && !o->unk_0b) {
                                int t = base + o->unk_0a;
                                if (t < tm) {
                                    int l2 = m;
                                    wtRow *r2 = (wtRow *)((cls_unk30 *)wt)->getIE_ck(m);
                                    while (l2 < wt->func_080741c0() && r2->unk_0c < t) {
                                        r2++;
                                        l2++;
                                    }
                                    if (l2 < wt->func_080741c0() && r2->unk_0c == t && r2->func_08072440(w, 0xff))
                                        ok = false;
                                }
                            }
                        }
                    }
                }
            }
            if (ok) {
                wtRow *r = row + 1;
                for (int l = rl + 1; l < m && ok; l++, r++) {
                    int base = r->unk_0c - 1;
                    for (int pp = 0; pp != 2; pp++) {
                        Instr *in = GetI((Instr **)r, pp);
                        if (in) {
                            short e = in->unk_13b2;
                            if (e < in->unk_13b0)
                                e = in->unk_13b0;
                            if (e > base) {
                                ok = false;
                                break;
                            }
                        }
                    }
                }
                if (ok) {
                    if (m >= wt->func_080741c0()) {
                        int last = wt->func_08072eec() - 1;
                        for (int l = wt->func_0807411c(); l < m && ok; l++) {
                            wtRow *r = (wtRow *)((cls_unk30 *)wt)->getIE_ck(l);
                            int dist = last - row->unk_0c;
                            if (dist < 0)
                                func_0804ae34("distance to end appears to be less than zero\n");
                            for (int pp = 0; pp != 2; pp++) {
                                Instr *in = GetI((Instr **)r, pp);
                                if (in) {
                                    short e = in->unk_13b6;
                                    if (e < in->unk_13b4)
                                        e = in->unk_13b4;
                                    if (e < dist) {
                                        ok = false;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (ok) {
                        if (m >= wt->func_080741c0()) {
                            int last = wt->func_08072eec() - 1;
                            if (last < wt->unk_44)
                                ok = false;
                            if (s->unk_2cd != 1 && last <= 1)
                                ok = false;
                        }
                        if (ok) {
                            row->unk_08 = 1;
                            splitCtime(wt, rl, m);
                            if (m >= wt->func_080741c0())
                                wt->unk_0c = --logLen;
                        }
                    }
                }
            }
        }
        int d = emuInst(p, row, rl, false, false);
        if (d < 0) {
            func_0804ad38("failed to emu an ientry\n");
            return false;
        }
        curTick += d;
        if (curTick != row->unk_0c + 1) {
            func_0804ad38("unexpected time !.. currTick=%d cTime=%d\n", curTick, row->unk_0c);
            return false;
        }
        if (curTick != p->unk_80) {
            func_0804ad38("currTick doesn't match cpu\n");
            return false;
        }
    }
    if (wt->func_08072eec() != p->unk_80) {
        func_0804ad38("at end of table logical len not correct\n");
        return false;
    }
    return true;
}

void func_0807bb84(workTable *wt)
{
    wtRow *row = wt->unk_2c + wt->unk_30;
    for (int l = wt->unk_30; l != wt->unk_34; l++, row++) {
        int rel = l - wt->unk_30;
        for (int p = 0; p != 2; p++) {
            Instr *in = GetI((Instr **)row, p);
            if (in && in->unk_1394 != rel)
                func_0804ae34("l=%d exec time mismatch in instruction %d!=%d\n",
                              l, in->unk_1394, rel);
        }
        row->unk_0c = l - wt->unk_30;
    }
}

void splitCtime(workTable *wt, int l, int m)
{
    if (l < wt->unk_30 || l >= wt->unk_34)
        func_0804ae34("nop line is bad\n");
    if (m <= l)
        func_0804ae34("m<l for splitCtime()\n");
    if (m > wt->unk_34)
        m = wt->unk_34;
    wtRow *row = &wt->unk_2c[l];
    for (; l < m; l++, row++)
        row->unk_0c--;
}

int getCtime(workTable *wt, int l)
{
    if (l < wt->unk_30 || l >= wt->unk_34)
        func_0804ae34("nop line is bad\n");
    return wt->unk_2c[l].unk_0c;
}

int earliestInsert(workTable *wt, Instr *in, bool *found)
{
    short e = in->unk_13b2;
    if (e < in->unk_13b0)
        e = in->unk_13b0;
    int line = e + wt->unk_30;
    int thru = -1;
    int iline = in->unk_13e8.unk_04;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = op->getReg();
        if (reg) {
            for (int p = 0; p != reg->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *slot = &op->unk_2c[p];
                    cls0805bcc4 *par = slot->getParent();
                    if (slot->unk_19 != 2 && par->unk_32 >= 0) {
                        int t = par->unk_32 + par->unk_18 + slot->unk_18;
                        if (t >= line) {
                            line = t;
                            if (op->unk_0b && !op->unk_29)
                                thru = t;
                        }
                        if (t > iline) {
                            func_0804ad38("ln>iline.. \n");
                            slot->print();
                            func_0804adf0(-1);
                        }
                    }
                }
            }
        }
        reg = op->getReg2();
        if (reg && reg->unk_2c) {
            for (int p = 0; p != reg->unk_28; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *slot = &op->unk_2c[p];
                    cls0805bcc4 *lk = slot->unk_04->unk_20;
                    while (lk && lk != slot) {
                        int s = lk->unk_32;
                        if (s > iline) {
                            func_0804ad38("sorted list corrupted?\n");
                            func_0804adf0(-1);
                        }
                        if (s >= 0) {
                            int t = s + lk->unk_17;
                            if (t >= line)
                                line = thru = t;
                            if (t > iline) {
                                func_0804ad38("ln>iline for thru case\n");
                                func_0804adf0(-1);
                            }
                        }
                        lk = lk->unk_10;
                    }
                    if (!lk) {
                        func_0804ad38("sorted lock not  on list for get earliest!\n");
                        func_0804adf0(-1);
                    }
                }
            }
        }
        if (op->getReg2() && op->unk_28)
            line = iline;
    }
    if (in->unk_1388)
        line = iline;
    if (found)
        *found = thru == line;
    return line;
}

int latestInsert(workTable *wt, Instr *in, bool *found)
{
    short e = in->unk_13b6;
    if (e < in->unk_13b4)
        e = in->unk_13b4;
    int line = wt->unk_34 - e - 1;
    int iline = in->unk_13e8.unk_04;
    bool thru = false;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = op->getReg();
        if (reg && op->unk_28) {
            thru = true;
            break;
        }
    }
    if (thru || in->unk_1388)
        line = iline;
    if (found)
        *found = false;
    return line;
}
