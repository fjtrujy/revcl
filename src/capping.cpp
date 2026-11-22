#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int insertStreamRegisterCaps(Program *p, instStream *s);
void initializeScheduledCycleTimes(workTable *wt);
bool removeNops(Program *p, instStream *s);
int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask);
bool doCap(int line, int pipe, int mask);
RegPool *GetRP(unsigned t);
bool resetProcessorThroughput(Program *p);
int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
void splitCtime(workTable *wt, int l, int m);

int insertAllRegisterCaps(Program *program)
{
    int total = 0;
    instStream *s;
    for (s = program->streamHead; s; s = s->nextStream) {
        s->externalSchedulingPrepared = 0;
        if (!s->usesFreeRawLayout())
            s->externalSchedulingPrepared = 1;
    }
    for (s = program->streamHead; s; s = s->nextStream) {
        if (s->externalSchedulingPrepared)
            continue;
        if (!prepExtInfo(program, s, 0)) {
            reportError("failed to prep external info for stream %s\n", s->streamName());
            return -1;
        }
        int n = insertStreamRegisterCaps(program, s);
        if (n < 0) {
            reportError("failed to sred a table from %s\n", s->streamName());
            return -1;
        }
        s->externalSchedulingPrepared = 1;
        total += n;
    }
    return total;
}

int insertStreamRegisterCaps(Program *program, instStream *stream)
{
    int added = 0;
    if (!stream->workTablePtr) {
        reportError("work table not set in %s\n", stream->streamName());
        exitAfterCleanup(-1);
    }
    workTable *wt = stream->workTablePtr;
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
    initializeScheduledCycleTimes(wt);
    for (int rl = wt->getRangeStart(); rl != wt->getRangeEnd(); rl++) {
        wtRow *row = (wtRow *)((cls_unk30 *)wt)->getIE_ck(rl);
        int id = row->ctime();
        for (int pp = 0; pp != 2; pp++) {
            Instr *in = GetI((Instr **)row, pp);
            if (!in)
                continue;
            Operand *operand;
            for (int i = 0; i != in->operandCount; i++) {
                operand = &in->operands[i];
                RegPoolEntry *reg = registerOperand(operand, 2);
                ol_s *live = operand->parts[0].operandLocations.head;
                if (!reg || operand->externalBlockModeFlag || !live)
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
                int need = operand->outputLatency;
                if (reg->registerIndex == 0x34 && need > 1)
                    need--;
                int line = rl + 1;
                wtRow *workRow = row + 1;
                int limit = need + id;
                int bestMask = 0, bestLine = -1, bestPipe = -1, bestCount = 0, count = 0;
                while (line < wt->getRangeEnd() && capMask) {
                    if (workRow->cycleTime >= limit) {
                        for (int q = 0; q != 2; q++) {
                            if (((bits >> q) & 1) && !GetI((Instr **)workRow, q)) {
                                if ((!workRow->countEntries() && bestLine < 0) || workRow->countEntries()) {
                                    bestLine = line;
                                    bestCount = count;
                                    bestPipe = q;
                                    bestMask = capMask;
                                }
                                break;
                            }
                        }
                    }
                    if (workRow->inputMaskFor(reg, 0xff))
                        break;
                    if (workRow->countEntries())
                        count++;
                    capMask &= ~workRow->outputMask(capReg);
                    line++;
                    workRow++;
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
