#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int insertStreamRegisterCaps(Program *p, instStream *s);
void func_0807bb84(workTable *wt);
bool removeNops(Program *p, instStream *s);
int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask);
bool doCap(int line, int pipe, int mask);
RegPool *GetRP(unsigned t);
bool resetProcessorThroughput(Program *p);
int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
void splitCtime(workTable *wt, int l, int m);

int insertAllRegisterCaps(Program *p)
{
    int total = 0;
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        s->externalSchedulingPrepared = 0;
        if (!s->usesFreeRawLayout())
            s->externalSchedulingPrepared = 1;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (s->externalSchedulingPrepared)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            reportError("failed to prep external info for stream %s\n", s->streamName());
            return -1;
        }
        int n = insertStreamRegisterCaps(p, s);
        if (n < 0) {
            reportError("failed to sred a table from %s\n", s->streamName());
            return -1;
        }
        s->externalSchedulingPrepared = 1;
        total += n;
    }
    return total;
}

int insertStreamRegisterCaps(Program *p, instStream *s)
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
    int len = wt->getRangeEnd() - wt->getRangeStart();
    if (len == 0)
        return 0;
    if (wt->externalMinimumLength < 0) {
        reportError("the minimal length is -ve for ext\n");
        return -1;
    }
    if (len < wt->externalMinimumLength) {
        reportError("table is lready too short (%d<%d)\n", len, wt->externalMinimumLength);
        return -1;
    }
    func_0807bb84(wt);
    for (int rl = wt->getRangeStart(); rl != wt->getRangeEnd(); rl++) {
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
                ol_s *live = op->parts[0].operandLocations.head;
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
                int need = op->outputLatency;
                if (reg->registerIndex == 0x34 && need > 1)
                    need--;
                int line = rl + 1;
                wtRow *r = row + 1;
                int limit = need + id;
                int bestMask = 0, bestLine = -1, bestPipe = -1, bestCount = 0, count = 0;
                while (line < wt->getRangeEnd() && capMask) {
                    if (r->cycleTime >= limit) {
                        for (int q = 0; q != 2; q++) {
                            if (((bits >> q) & 1) && !GetI((Instr **)r, q)) {
                                if ((!r->countEntries() && bestLine < 0) || r->countEntries()) {
                                    bestLine = line;
                                    bestCount = count;
                                    bestPipe = q;
                                    bestMask = capMask;
                                }
                                break;
                            }
                        }
                    }
                    if (r->inputMaskFor(reg, 0xff))
                        break;
                    if (r->countEntries())
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
