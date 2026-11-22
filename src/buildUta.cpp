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


bool buildUta(Program *prog, instStream *targetStream, DecodedLine *blocks, int count)
{
    workTable *scheduleTable = &targetStream->sourceTable;
    if (!scheduleTable) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!scheduleTable->init(1, 0)) {
        reportError("failed to init uta table for '%s'\n", targetStream->streamName());
        return false;
    }
    scheduleTable->ownerStream = targetStream;
    int flags = 0;
    int branch = -1;
    for (int decodedLineIndex = 0; decodedLineIndex != count; decodedLineIndex++) {
        DecodedLine *decodedLine = &blocks[decodedLineIndex];
        if (decodedLine->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        decodedLine->labels.clearNodes();
        Instr *decodedInstruction = decodedLine->instructions[0];
        if (!decodedInstruction)
            continue;
        if (decodedInstruction->parsedLoopId > 0) {
            reportUserMessage("parseLid set for a uta instruction\n");
            decodedInstruction->leDump();
            return false;
        }
        decodedInstruction->parsedLoopId = -1;
        if (decodedInstruction->syntheticKind) {
            if (!targetStream->processCmdSynth(decodedInstruction)) {
                reportUserMessage("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                dumpUtaInstruction(decodedInstruction);
                errorOccurred = saved;
            }
            flags |= decodedInstruction->instructionModifierBits;
        } else {
            decodedLine->instructions[0] = 0;
            if (!scheduleTable->logAddLinear_Tail(decodedInstruction)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            if (decodedInstruction->branchFlags) {
                if (branch >= 0) {
                    reportError("more than one branch in uta decode\n");
                    return false;
                }
                branch = decodedInstruction->branchFlags;
            }
        }
    }
    if (branch < 0)
        branch = 0;
    targetStream->branchType = branch;
    if (flags)
        reportInfo("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
}

int branchLinkTypeForFlags(int flags);
void dumpProgramStreamsAndRegisters(Program *p, int a);
bool fixupBtorBranch(Program *p, instStream *s, int *changed);
