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

static inline void dumpUtaInstruction(Instr *in)
{
    in->dumpOperandDiagnostics(false);
}


bool buildUta(Program *prog, instStream *s, DecodedLine *blocks, int count)
{
    workTable *t = &s->sourceTable;
    if (!t) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(1, 0)) {
        reportError("failed to init uta table for '%s'\n", s->streamName());
        return false;
    }
    t->ownerStream = s;
    int flags = 0;
    int branch = -1;
    for (int i = 0; i != count; i++) {
        DecodedLine *b = &blocks[i];
        if (b->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->labels.clearNodes();
        Instr *in = b->instructions[0];
        if (!in)
            continue;
        if (in->parsedLoopId > 0) {
            reportUserMessage("parseLid set for a uta instruction\n");
            in->leDump();
            return false;
        }
        in->parsedLoopId = -1;
        if (in->syntheticKind) {
            if (!s->processCmdSynth(in)) {
                reportUserMessage("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                dumpUtaInstruction(in);
                errorOccurred = saved;
            }
            flags |= in->instructionModifierBits;
        } else {
            b->instructions[0] = 0;
            if (!t->logAddLinear_Tail(in)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            if (in->branchFlags) {
                if (branch >= 0) {
                    reportError("more than one branch in uta decode\n");
                    return false;
                }
                branch = in->branchFlags;
            }
        }
    }
    if (branch < 0)
        branch = 0;
    s->branchType = branch;
    if (flags)
        reportInfo("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
}

int branchLinkTypeForFlags(int flags);
void dumpProgramStreamsAndRegisters(Program *p, int a);
bool fixupBtorBranch(Program *p, instStream *s, int *changed);
