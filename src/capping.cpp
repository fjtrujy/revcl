#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int func_0807aaf4(Program *p, instStream *s);
void func_0807bb84(workTable *wt);
bool removeNops(Program *p, instStream *s);
int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask);
bool doCap(int line, int pipe, int mask);
RegPool *GetRP(unsigned t);
bool func_0806c0f8(Program *p);
int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
void splitCtime(workTable *wt, int l, int m);

int func_0807aa28(Program *p)
{
    int total = 0;
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        s->unk_2dc = 0;
        if (!s->func_0806d9a0())
            s->unk_2dc = 1;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (s->unk_2dc)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            reportError("failed to prep external info for stream %s\n", s->streamName());
            return -1;
        }
        int n = func_0807aaf4(p, s);
        if (n < 0) {
            reportError("failed to sred a table from %s\n", s->streamName());
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
    if (!s->workTablePtr) {
        reportError("work table not set in %s\n", s->streamName());
        exitAfterCleanup(-1);
    }
    workTable *wt = s->workTablePtr;
    if (!wt) {
        reportError("no work table \n");
        return -1;
    }
    int len = wt->func_080741c0() - wt->func_0807411c();
    if (len == 0)
        return 0;
    if (wt->unk_44 < 0) {
        reportError("the minimal length is -ve for ext\n");
        return -1;
    }
    if (len < wt->unk_44) {
        reportError("table is lready too short (%d<%d)\n", len, wt->unk_44);
        return -1;
    }
    func_0807bb84(wt);
    for (int rl = wt->func_0807411c(); rl != wt->func_080741c0(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)wt)->getIE_ck(rl);
        int id = row->ctime();
        for (int pp = 0; pp != 2; pp++) {
            Instr *in = GetI((Instr **)row, pp);
            if (!in)
                continue;
            Operand *op;
            for (int i = 0; i != in->operandCount; i++) {
                op = &in->operands[i];
                RegPoolEntry *reg = registerOperand(op, 2);
                ol_s *live = op->unk_2c[0].unk_54.head;
                if (!reg || op->unk_0b || !live)
                    continue;
                RegPoolEntry *capReg = 0;
                int capMask = 0;
                int bits = findCap(wt, in, i, &capReg, &capMask);
                if (bits < 0) {
                    reportError("cancap failed\n");
                    return -1;
                }
                if (!bits)
                    continue;
                int need = op->unk_0a;
                if (reg->registerIndex == 0x34 && need > 1)
                    need--;
                int line = rl + 1;
                wtRow *r = row + 1;
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
                    capMask &= ~r->outputMask(capReg);
                    line++;
                    r++;
                }
                if (bestLine < 0)
                    continue;
                if (bestCount < need) {
                    if (!doCap(bestLine, bestPipe, bestMask)) {
                        reportError("failed to add cap instruction\n");
                        return -1;
                    }
                    added++;
                }
            }
        }
    }
    if (added && !wt->relinkRawIt()) {
        reportError("failed to relink table after adding cap instruction\n");
        return 0;
    }
    return added;
}
