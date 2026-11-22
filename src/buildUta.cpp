#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quietDiagnostics;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

static inline void dumpUtaInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}


bool buildUta(Program *prog, instStream *s, DecodedLine *blocks, int count)
{
    workTable *scheduleTable = &s->sourceTable;
    if (!scheduleTable) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!scheduleTable->init(1, 0)) {
        reportError("failed to init uta table for '%s'\n", s->streamName());
        return false;
    }
    scheduleTable->ownerStream = s;
    int flags = 0;
    int branch = -1;
    for (int i = 0; i != count; i++) {
        DecodedLine *decodedLine = &blocks[i];
        if (decodedLine->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        decodedLine->labels.clearNodes();
        Instr *in = decodedLine->instructions[0];
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
                errorOccurred = !quietDiagnostics;
                dumpUtaInstruction(in);
                errorOccurred = saved;
            }
            flags |= in->instructionModifierBits;
        } else {
            decodedLine->instructions[0] = 0;
            if (!scheduleTable->logAddLinear_Tail(in)) {
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
