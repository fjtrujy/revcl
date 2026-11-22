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

static inline void dumpRow(wtRow *row)
{
    row->refreshInstructionOperands(-1);
}

bool buildRaw(Program *prog, instStream *targetStream, DecodedLine *blocks, int count, int loopMode)
{
    workTable *scheduleTable = &targetStream->sourceTable;
    if (!scheduleTable) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!scheduleTable->init(5, count)) {
        reportError("failed to init raw table for '%s' with %d entries\n", targetStream->streamName(), count);
        return false;
    }
    scheduleTable->ownerStream = targetStream;
    if (!scheduleTable->setTableRawMode(loopMode)) {
        reportError("failed to set sub mode\n");
        return false;
    }
    int decodedLineIndex = 0;
    int rows = 0;
    bool looped = false;
    Instr *branch = 0;
    int branchRow = -1;
    int branchType = 0;
    for (; decodedLineIndex != count; decodedLineIndex++) {
        DecodedLine *decodedLine = &blocks[decodedLineIndex];
        if (decodedLine->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        decodedLine->labels.clearNodes();
        Instr *syntheticInstruction = decodedLine->instructions[0];
        if (syntheticInstruction && syntheticInstruction->syntheticKind) {
            if (!targetStream->processCmdSynth(syntheticInstruction)) {
                reportUserMessage("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                syntheticInstruction->dumpOperandDiagnostics(false);
                errorOccurred = saved;
            }
            continue;
        }
        wtRow *row = scheduleTable->getRowByIndex(rows);
        if (!row) {
            reportError("failed to get ied\n");
            return false;
        }
        row->clearEntries();
        int instructionCount = 0;
        int valid = 0;
        for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
            Instr *decodedInstruction = decodedLine->instructions[instructionIndex];
            if (!decodedInstruction)
                continue;
            if (decodedInstruction->syntheticKind) {
                reportError("encountered a synth instruction in raw mode\n");
                return false;
            }
            decodedLine->instructions[instructionIndex] = 0;
            if (!scheduleTable->logAddLinear_Tail(decodedInstruction)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            short lid = decodedInstruction->parsedLoopId;
            if (lid < 0) {
                reportError("-ve parseLid set\n");
                return false;
            }
            if (lid) {
                if (decodedInstruction->isNop()) {
                    reportUserMessage("loopid set on a nop\n");
                    decodedInstruction->leDump();
                    return false;
                }
                if (!looped && valid) {
                    reportUserMessage("flow::build - hit a looped vsm instruction - but others before it weren't..%d valid \n", valid);
                    decodedInstruction->leDump();
                    return false;
                }
                looped = true;
            } else if (looped && !decodedInstruction->isNop()) {
                reportError("flow::build - parse id wasn't set for a supposedly looped instruction.type\n");
                decodedInstruction->leDump();
                return false;
            }
            if (decodedInstruction->getBranchType()) {
                if (branchRow != -1) {
                    reportError("found more than one branch..in what should be continuous sequence\n");
                    return false;
                }
                branchType = decodedInstruction->getBranchType();
                branchRow = rows;
                branch = decodedInstruction;
            }
            row->entries[instructionIndex].instruction = decodedInstruction;
            decodedInstruction->setOwner((cls_unk30 *)scheduleTable);
            valid++;
            instructionCount++;
        }
        if (instructionCount) {
            if (checkDispatchEntry(&row->entries[0].instruction)) {
                reportUserMessage("invalid instruction combination\n");
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                dumpRow(row);
                errorOccurred = saved;
                return false;
            }
            rows++;
        }
    }
    targetStream->branchType = branchType;
    if (branchRow >= 0) {
        int branchDelayRows = rows - branchRow - 1;
        if (branchDelayRows <= 0) {
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
        if (branchDelayRows > 1) {
            reportError("stray branch instruction found in sequence\n");
            branch->leDump();
            return false;
        }
    }
    if (looped)
        scheduleTable->setRange(rows);
    else
        scheduleTable->setRange(0, rows);
    setRangeFrom(scheduleTable);
    int maxLid = 0;
    for (decodedLineIndex = 0; decodedLineIndex != rows; decodedLineIndex++) {
        wtRow *row = scheduleTable->getRowByIndex(decodedLineIndex);
        if (!row) {
            reportError("failed to get ie from table\n");
            return false;
        }
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *rowInstruction = row->entries[entryIndex].instruction;
            if (!rowInstruction)
                continue;
            int lid, pos;
            if (looped) {
                lid = rowInstruction->parsedLoopId - 1;
                pos = decodedLineIndex + rows * lid;
            } else {
                pos = decodedLineIndex;
                lid = 0;
            }
            rowInstruction->parsedLoopId = -1;
            if (lid > maxLid)
                maxLid = lid;
            rowInstruction->position.setInstructionPipe(entryIndex);
            rowInstruction->position.setFromLinearRow(pos);
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
    scheduleTable->mainLoopId = maxLid;
    resetRange();
    return true;
}
