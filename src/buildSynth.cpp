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

static inline void dumpSynthInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}


bool buildSynth(Program *program, instStream *targetStream, DecodedLine *blocks, int count)
{
    workTable *scheduleTable = &targetStream->sourceTable;
    if (!scheduleTable) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!scheduleTable->init(4, 0)) {
        reportError("failed to init synth table for '%s'\n", targetStream->streamName());
        return false;
    }
    scheduleTable->ownerStream = targetStream;
    int inSynth;
    int stype;
    int flags = 0;
    inSynth = 0;
    stype = 0;
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
        bool isIO = false;
        if (decodedInstruction->parsedLoopId > 0) {
            reportUserMessage("parseLid set for a synth instruction\n");
            decodedInstruction->dumpInstructionContext();
            return false;
        }
        decodedInstruction->parsedLoopId = -1;
        switch ((unsigned char)decodedInstruction->syntheticKind) {
        case 11:
            if (targetStream->endDirectiveText) {
                reportUserMessage("istream already has end macro attached\n");
                return false;
            }
            if (!decodedInstruction->operands[0].immediateValue.getString()) {
                reportError("EXITM has no macro string attached\n");
                return false;
            }
            targetStream->endDirectiveText = dupString(decodedInstruction->operands[0].immediateValue.getString());
        case 1:
        case 3:
            if (stype) {
                const char *syntheticName = syntheticTypeName((unsigned char)decodedInstruction->syntheticKind);
                reportError("flow related synthetic (%s) encountered.. but stype not null (%s)!\n", syntheticName, syntheticTypeName(stype));
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            inSynth = stype = (unsigned char)decodedInstruction->syntheticKind;
            break;
        case 2:
            if (stype != 1) {
                reportError("unexpected --endenter encountered\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            if (!inSynth) {
                reportError("flow related synthetic encountered.. but the block didn't appear to be wholly synthetic\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            inSynth = 0;
            break;
        case 4:
            if (!inSynth) {
                reportError("end exit encountered, but not in synthetic mode\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            if (stype != 3 && stype != 11) {
                reportError("unexpected --endexit encountered\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            inSynth = 0;
            break;
        case 6:
            isIO = true;
            if (stype != 1) {
                reportUserMessage("synthetic input not in enter block\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            break;
        case 7:
            if (stype != 3 && stype != 11) {
                reportUserMessage("synthetic output not in exit block\n");
                decodedInstruction->dumpInstructionContext();
                return false;
            }
            isIO = true;
            break;
        default:
            reportUserMessage("encountered an misplaced instruction in a enter or exit block\n");
            dumpSynthInstruction(decodedInstruction);
            return false;
        }
        if (isIO) {
            decodedLine->instructions[0] = 0;
            if (!scheduleTable->logAddLinear_Tail(decodedInstruction)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
        } else
            flags |= decodedInstruction->instructionModifierBits;
    }
    switch (stype) {
    case 11:
        targetStream->endType = 4;
        if (!targetStream->endDirectiveText) {
            reportError("exitmacro not set.. but end_macro endtype?\n");
            return false;
        }
        break;
    case 3:
        targetStream->endType = 3;
        break;
    case 1:
        targetStream->startType = 2;
        break;
    default:
        reportUserMessage("vcl found a problem making an enter/exit block, and I'll improve this error message later\n");
        return false;
    }
    if (flags)
        reportInfo("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
}
