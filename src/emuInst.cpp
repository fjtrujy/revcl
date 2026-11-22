#include "emu_shared.h"

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "ol_s_inline.h"

int emulateInstructionRow(Program *p, ScheduleRow *row, int line, bool uta, bool commit);
bool advanceProcessorCycle(Program *p, bool flag);


int emulateInstructionRow(Program *program, ScheduleRow *row, int line, bool uta, bool commit)
{
    int entryIndex;
    int stalls = 0;
    Instr *activeInstruction;
    Operand *activeOperand;
    RegisterEntry *registerEntry;
    if (!row) {
        reportError("passed a null pointer to emuVSM\n");
        return -1;
    }
    if (row->skip)
        return 0;
    if (uta && !advanceProcessorCycle(program, uta)) {
        reportError("Processor::Emu - failed initial cycle for uta!\n");
        return -1;
    }

    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        Instr *rowInstruction = row->entries[entryIndex].instruction;
        if (!rowInstruction)
            continue;
        if (commit && rowInstruction->previousThroughputWriter) {
            reportError("previous thruput write already set for instruction\n");
            return -1;
        }
        int lid = (signed char)rowInstruction->position.loopIteration;
        if (uta) {
            if (lid != -1) {
                reportError("lid>=0 for uta mode emu\n");
                return -1;
            }
        } else if (lid) {
            reportError("emu - called with an instruction with lid non-zero for vsm..%d\n", lid);
            return -1;
        }
    }
    int maxLat = 0, maxThru = 0;
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        activeInstruction = row->entries[entryIndex].instruction;
        if (!activeInstruction)
            continue;
        for (int operandIndex = 0; (activeOperand = activeInstruction->getOp(operandIndex)) != 0; operandIndex++) {
            registerEntry = registerOperand(activeOperand, 1);
            if (registerEntry) {
                for (int part = 0; part != registerEntry->partCount; part++)
                    if ((activeOperand->componentMask >> part) & 1) {
                        RegisterPartState *registerPart = &registerEntry->parts[part];
                        if (registerPart->emulationTimingMetrics[4] < 0) {
                            if (uta)
                                registerPart->emulationTimingMetrics[6] = -1;
                            else
                                registerPart->emulationTimingMetrics[6] = program->emulatedCycleCount;
                            registerPart->emulationTimingMetrics[4] = line;
                            registerPart->emulationTimingMetrics[5] = entryIndex;
                        }
                        registerPart->emulationTimingMetrics[7] = line;
                        registerPart->emulationTimingMetrics[8] = entryIndex;
                    }
            }
            if (registerEntry && activeOperand->externalBlockModeFlag) {
                for (int part = 0; part != registerEntry->partCount; part++)
                    if ((activeOperand->componentMask >> part) & 1) {
                        RegisterPartState *inputRegisterPart = &registerEntry->parts[part];
                        int lat;
                        if (inputRegisterPart->pendingSource && (lat = inputRegisterPart->pendingSource->cycleOffset) != 0) {
                            OperandLocation temporaryLocation;
                            temporaryLocation.init(activeInstruction, operandIndex, part, false);
                            if (commit && !program->proc.stalls.addStall(inputRegisterPart->pendingSource, &temporaryLocation, 0, lat)) {
                                reportError("failed to add an internal latency stall\n");
                                return -1;
                            }
                            if (activeOperand->additionalLatency) {
                                reportError("vcl detected an ialu op too close to a dependent conditional branch. target behaviour isn't guaranteed\n");
                                activeInstruction->dumpInstructionContext();
                                return -1;
                            }
                            if (lat > maxLat)
                                maxLat = lat;
                        }
                    }
            }
            registerEntry = registerOperand(activeOperand, 2);
            if (registerEntry) {
                if (activeOperand->componentMask) {
                    if (registerEntry->tracksHotOutputPaths) {
                        if (commit) {
                            if (activeInstruction->previousThroughputWriter) {
                                reportError("multiple set for prevThruput\n");
                                return -1;
                            }
                            activeInstruction->previousThroughputWriter = registerEntry->throughputWriter;
                        } else if (registerEntry->throughputWriter != activeInstruction->previousThroughputWriter) {
                            reportError("unexpected value found in prev thru write\n");
                            return -1;
                        }
                    }
                    registerEntry->throughputWriter = activeInstruction;
                    registerEntry->outputWriteCount++;
                }
                for (int part = 0; part != registerEntry->partCount; part++) {
                    if ((activeOperand->componentMask >> part) & 1) {
                        RegisterPartState *inputRegisterPart = &registerEntry->parts[part];
                        if (inputRegisterPart->issueLock) {
                            OperandLocation inputPendingLocation;
                            inputPendingLocation.init(activeInstruction, operandIndex, part, false);
                            if (commit) {
                                int issueLock = inputRegisterPart->issueLock;
                                if (!program->proc.stalls.addStall(inputRegisterPart->pendingSource, &inputPendingLocation, 1, issueLock)) {
                                    reportError("failed to add an internal thruput stall\n");
                                    return -1;
                                }
                            }
                            maxThru = inputRegisterPart->issueLock;
                        }
                    }
                }
            }
        }
    }
    int cyc;
    if (uta) {
        if (maxLat || maxThru) {
            reportError("in uta mode, but encountered latency / throughput stall\n");
            return -1;
        }
        cyc = -1;
    } else {
        int stall = 0;
        if (maxLat)
            stall = maxLat;
        if (maxThru && maxThru > stall)
            stall = maxThru;
        if (stall) {
            stalls = stall;
            do {
                if (!advanceProcessorCycle(program, uta)) {
                    reportError("failed to cycle cpu\n");
                    exitAfterCleanup(-1);
                }
            } while (--stall);
        }
        cyc = program->emulatedCycleCount;
    }
    for (entryIndex = 0; entryIndex != 2; entryIndex++)
        if (row->entries[entryIndex].instruction)
            row->entries[entryIndex].instruction->scheduledCycle = cyc;

    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        activeInstruction = row->entries[entryIndex].instruction;
        if (!activeInstruction)
            continue;
        for (int operandIndex = 0; (activeOperand = activeInstruction->getOp(operandIndex)) != 0; operandIndex++) {
            registerEntry = registerOperand(activeOperand, 1);
            if (registerEntry) {
                for (int part = 0; part != registerEntry->partCount; part++) {
                    if ((activeOperand->componentMask >> part) & 1) {
                        RegisterPartState *setterRegisterPart = &registerEntry->parts[part];
                        OperandLocation *setter = &setterRegisterPart->currentLocation;
                        Instr *sourceInstruction = setter->instruction;
                        if (!sourceInstruction) {
                            activeOperand->externalPartMask |= 1 << part;
                            continue;
                        }
                        Operand *sourceOperand = operandAtLocation(setter);
                        if (!sourceOperand || !isRegisterOperand(sourceOperand, 2)) {
                            reportError("emuUTA: operand that wasn't output set input\n");
                            return -1;
                        }
                        if (!((sourceOperand->componentMask >> part) & 1)) {
                            reportError("we used something not really set by setter\n");
                            return -1;
                        }
                        if (commit) {
                            int sourceOp = setterRegisterPart->currentLocation.operandIndex;
                            if (!linkOperandParts(sourceInstruction, sourceOp, part, activeInstruction, operandIndex, part)) {
                                reportError("failed to cross link operands\n");
                                return -1;
                            }
                        }
                    }
                }
            }
        }
    }
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        Instr *currentInstruction = row->entries[entryIndex].instruction;
        if (!currentInstruction)
            continue;
        for (int operandIndex = 0; (activeOperand = currentInstruction->getOp(operandIndex)) != 0; operandIndex++) {
            registerEntry = registerOperand(activeOperand, 2);
            if (registerEntry) {
                for (int part = 0; part != registerEntry->partCount; part++) {
                    if ((activeOperand->componentMask >> part) & 1) {
                        RegisterPartState *outputRegisterPart = &registerEntry->parts[part];
                        if (outputRegisterPart->issueLock) {
                            reportError("inconsistency detected.. issuelock still non-zero\n");
                            return -1;
                        }
                        if (activeOperand->externalBlockModeFlag) {
                            if (!outputRegisterPart->flushPending()) {
                                reportError("failed to flush pending outputs before a blocking output\n");
                                return -1;
                            }
                            OperandLocation *location = &outputRegisterPart->currentLocation;
                            location->init(currentInstruction, operandIndex, part, false);
                            location->cycleOffset = clampByte(activeOperand->outputLatency);
                            outputRegisterPart->pendingSource = location;
                            outputRegisterPart->cycleAge = 0;
                        } else {
                            OperandLocation *pendingLocation = outputRegisterPart->acquireFreePendingLocation();
                            if (!pendingLocation) {
                                reportError("EMU: failed to get OL\n");
                                return -1;
                            }
                            pendingLocation->init(currentInstruction, operandIndex, part, false);
                            pendingLocation->cycleOffset = clampByte(activeOperand->outputLatency);
                            outputRegisterPart->pendingSource = pendingLocation;
                        }
                        outputRegisterPart->emulationTimingMetrics[1] += activeOperand->outputLatency;
                        if (activeOperand->outputLatency > outputRegisterPart->emulationTimingMetrics[3])
                            outputRegisterPart->emulationTimingMetrics[3] = activeOperand->outputLatency;
                        int throughputInterval = 1;
                        if (registerEntry->tracksHotOutputPaths)
                            throughputInterval = currentInstruction->baseIssueSpacing;
                        if (throughputInterval > outputRegisterPart->emulationTimingMetrics[2])
                            outputRegisterPart->emulationTimingMetrics[2] = throughputInterval;
                        if (outputRegisterPart->emulationTimingMetrics[9] < 0) {
                            outputRegisterPart->emulationTimingMetrics[9] = line;
                            outputRegisterPart->emulationTimingMetrics[10] = entryIndex;
                            if (uta)
                                outputRegisterPart->emulationTimingMetrics[11] = -1;
                            else
                                outputRegisterPart->emulationTimingMetrics[11] = program->emulatedCycleCount;
                        }
                        outputRegisterPart->emulationTimingMetrics[12] = line;
                        outputRegisterPart->emulationTimingMetrics[13] = entryIndex;
                        if (uta)
                            outputRegisterPart->emulationTimingMetrics[14] = -1;
                        else
                            outputRegisterPart->emulationTimingMetrics[14] = program->emulatedCycleCount;
                        outputRegisterPart->emulationTimingMetrics[16] = activeOperand->outputLatency;
                        outputRegisterPart->emulationTimingMetrics[15] = throughputInterval;
                        outputRegisterPart->emulationTimingMetrics[0] += throughputInterval;
                        outputRegisterPart->issueLock = throughputInterval;
                    }
                }
            }
        }
    }
    stalls++;
    if (!advanceProcessorCycle(program, uta)) {
        reportError("failed to cycle cpu!\n");
        return -1;
    }
    return stalls;
}
