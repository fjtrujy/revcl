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

int func_0807aa28(Program *p)
{
    int total = 0;
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        s->unk_2dc = 0;
        if (!s->func_0806d9a0())
            s->unk_2dc = 1;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2dc)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            func_0804ad38("failed to prep external info for stream %s\n", s->func_0806d674());
            return -1;
        }
        int n = func_0807aaf4(p, s);
        if (n < 0) {
            func_0804ad38("failed to sred a table from %s\n", s->func_0806d674());
            return -1;
        }
        s->unk_2dc = 1;
        total += n;
    }
    return total;
}

int func_0807aaf4(Program *p, instStream *s)
{
    int added = 0;
    if (!s->unk_218) {
        func_0804ad38("work table not set in %s\n", s->func_0806d674());
        func_0804adf0(-1);
    }
    workTable *wt = s->unk_218;
    if (!wt) {
        func_0804ad38("no work table \n");
        return -1;
    }
    int len = wt->func_080741c0() - wt->func_0807411c();
    if (len == 0)
        return 0;
    if (wt->unk_44 < 0) {
        func_0804ad38("the minimal length is -ve for ext\n");
        return -1;
    }
    if (len < wt->unk_44) {
        func_0804ad38("table is lready too short (%d<%d)\n", len, wt->unk_44);
        return -1;
    }
    func_0807bb84(wt);
    for (int rl = wt->func_0807411c(); rl != wt->func_080741c0(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)wt)->getIE_ck(rl);
        int id = row->unk_0c;
        for (int pp = 0; pp != 2; pp++) {
            Instr *in = GetI((Instr **)row, pp);
            if (!in)
                continue;
            Operand *op = in->unk_38;
            for (int i = 0; i != in->unk_138b; i++, op++) {
                ol_s *live = op->unk_2c[0].unk_54.unk_00;
                RegPoolEntry *reg = op->getReg2();
                if (!reg || op->unk_0b || !live)
                    continue;
                RegPoolEntry *capReg = 0;
                int capMask = 0;
                int bits = findCap(wt, in, i, &capReg, &capMask);
                if (bits < 0) {
                    func_0804ad38("cancap failed\n");
                    return -1;
                }
                if (!bits)
                    continue;
                int need = op->unk_0a;
                if (reg->unk_30 == 0x34 && need > 1)
                    need--;
                wtRow *r = row + 1;
                int line = rl + 1;
                int limit = need + id;
                int bestMask = 0, bestLine = -1, bestPipe = -1, bestCount = 0, count = 0;
                while (line < wt->func_080741c0() && capMask) {
                    if (r->unk_0c >= limit) {
                        for (int q = 0; q != 2; q++) {
                            if (((bits >> q) & 1) && !GetI((Instr **)r, q)) {
                                if ((!r->func_08072304() && bestLine < 0) || r->func_08072304()) {
                                    bestLine = line;
                                    bestCount = count;
                                    bestPipe = q;
                                    bestMask = capMask;
                                }
                                break;
                            }
                        }
                    }
                    if (r->func_08072440(reg, 0xff))
                        break;
                    if (r->func_08072304())
                        count++;
                    capMask &= ~r->func_080723ec(capReg, 0xff);
                    line++;
                    r++;
                }
                if (bestLine < 0)
                    continue;
                if (bestCount < need) {
                    if (!doCap(bestLine, bestPipe, bestMask)) {
                        func_0804ad38("failed to add cap instruction\n");
                        return -1;
                    }
                    added++;
                }
            }
        }
    }
    if (added && !wt->relinkRawIt()) {
        func_0804ad38("failed to relink table after adding cap instruction\n");
        return 0;
    }
    return added;
}

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
