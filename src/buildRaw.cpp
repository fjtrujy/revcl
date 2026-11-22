#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

static inline void dumpRow(wtRow *row)
{
    row->refreshInstructionOperands(-1);
}

bool buildRaw(Program *prog, instStream *s, DecodedLine *blocks, int count, int loopMode)
{
    workTable *t = &s->sourceTable;
    if (!t) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(5, count)) {
        reportError("failed to init raw table for '%s' with %d entries\n", s->streamName(), count);
        return false;
    }
    t->ownerStream = s;
    if (!t->setTableRawMode(loopMode)) {
        reportError("failed to set sub mode\n");
        return false;
    }
    int i = 0;
    int rows = 0;
    bool looped = false;
    Instr *branch = 0;
    int branchRow = -1;
    int branchType = 0;
    for (; i != count; i++) {
        DecodedLine *b = &blocks[i];
        if (b->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->labels.clearNodes();
        Instr *in = b->instructions[0];
        if (in && in->syntheticKind) {
            if (!s->processCmdSynth(in)) {
                reportUserMessage("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                in->dumpOperandDiagnostics(false);
                errorOccurred = saved;
            }
            continue;
        }
        wtRow *row = t->getRowByIndex(rows);
        if (!row) {
            reportError("failed to get ied\n");
            return false;
        }
        row->clearEntries();
        int n = 0;
        int valid = 0;
        for (int j = 0; j != 2; j++) {
            Instr *in = b->instructions[j];
            if (!in)
                continue;
            if (in->syntheticKind) {
                reportError("encountered a synth instruction in raw mode\n");
                return false;
            }
            b->instructions[j] = 0;
            if (!t->logAddLinear_Tail(in)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            short lid = in->parsedLoopId;
            if (lid < 0) {
                reportError("-ve parseLid set\n");
                return false;
            }
            if (lid) {
                if (in->isNop()) {
                    reportUserMessage("loopid set on a nop\n");
                    in->leDump();
                    return false;
                }
                if (!looped && valid) {
                    reportUserMessage("flow::build - hit a looped vsm instruction - but others before it weren't..%d valid \n", valid);
                    in->leDump();
                    return false;
                }
                looped = true;
            } else if (looped && !in->isNop()) {
                reportError("flow::build - parse id wasn't set for a supposedly looped instruction.type\n");
                in->leDump();
                return false;
            }
            if (in->getBranchType()) {
                if (branchRow != -1) {
                    reportError("found more than one branch..in what should be continuous sequence\n");
                    return false;
                }
                branchType = in->getBranchType();
                branchRow = rows;
                branch = in;
            }
            row->entries[j].unk_00 = in;
            in->setOwner((cls_unk30 *)t);
            valid++;
            n++;
        }
        if (n) {
            if (checkDispatchEntry(&row->entries[0].unk_00)) {
                reportUserMessage("invalid instruction combination\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                dumpRow(row);
                errorOccurred = saved;
                return false;
            }
            rows++;
        }
    }
    s->branchType = branchType;
    if (branchRow >= 0) {
        int d = rows - branchRow - 1;
        if (d <= 0) {
            if (looped) {
                reportError("in raw_loop mode, vcl expects you to set the branch delay slot!\n");
                branch->leDump();
                return false;
            }
            reportUserMessage("found a raw sequence without a bdl slot instruction.. this could be because mode\n");
            reportUserMessage("flips to UTA , or it could be the bdl slot instruction is also a branch target\n");
            reportUserMessage("for now -this is an error\n");
            return false;
        }
        if (d > 1) {
            reportError("stray branch instruction found in sequence\n");
            branch->leDump();
            return false;
        }
    }
    if (looped)
        t->setRange(rows);
    else
        t->setRange(0, rows);
    setRangeFrom(t);
    int maxLid = 0;
    for (i = 0; i != rows; i++) {
        wtRow *row = t->getRowByIndex(i);
        if (!row) {
            reportError("failed to get ie from table\n");
            return false;
        }
        for (int j = 0; j != 2; j++) {
            Instr *in = row->entries[j].unk_00;
            if (!in)
                continue;
            int lid, pos;
            if (looped) {
                lid = in->parsedLoopId - 1;
                pos = i + rows * lid;
            } else {
                pos = i;
                lid = 0;
            }
            in->parsedLoopId = -1;
            if (lid > maxLid)
                maxLid = lid;
            in->position.setInstructionPipe(j);
            in->position.setFromLinearRow(pos);
        }
    }
    if (looped && !maxLid) {
        reportUserMessage("max loop id 0 in loop\n");
        return false;
    }
    if (maxLid && !looped) {
        reportUserMessage("max lid not zero and not in a loop\n");
        return false;
    }
    t->mainLoopId = maxLid;
    resetRange();
    return true;
}
