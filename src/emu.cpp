#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "ol_s_inline.h"

int emuInst(Program *p, wtRow *row, int line, bool uta, bool commit);
bool Cycle(Program *p, bool flag);

static inline int clampByte(int value)
{
    return value > 255 ? 255 : value;
}

bool emuTable(Program *program, workTable *scheduleTable)
{
    if (!scheduleTable) {
        reportWarning("emu - passed a null table ptr!\n");
        return true;
    }
    if (scheduleTable->getLoopSize() || scheduleTable->mainLoopId) {
        reportError("mlid is set, or loop size is set for emu..which is now kinda tricky..\n");
        return false;
    }
    scheduleTable->declaredRawLength = -1;
    switch (scheduleTable->codeModeValue) {
    case 3: {
        int skipped = 0;
        for (int line = scheduleTable->getRangeStart(); line != scheduleTable->getRangeEnd(); line++) {
            wtRow *workRow = scheduleTable->getRowByIndex(line);
            if (workRow) {
                if (workRow->skip) {
                    for (int entryIndex = 0; entryIndex != 2; entryIndex++)
                        if (workRow->entries[entryIndex].instruction) {
                            reportError("m_bSkip set .. but line contains instructions\n");
                            return false;
                        }
                    skipped++;
                    continue;
                }
                if (emuInst(program, workRow, line, false, true) < 0) {
                    reportError("failed to Emu an inst\n");
                    workRow->refreshInstructionOperands(-1);
                    goto fail;
                }
            } else {
                reportError("emuuta - failed to get an ie\n");
                return false;
            }
        }
        scheduleTable->declaredRawLength = program->emulatedCycleCount;
        skipped += program->emulatedCycleCount;
        if (skipped < scheduleTable->getPhysicalLen()) {
            reportError("logical len < physical len.. %d<%d!\n", program->emulatedCycleCount, scheduleTable->getPhysicalLen());
            scheduleTable->dump();
            return false;
        }
        break;
    }
fail:
    return false;
    case 1:
    case 4: {
        Instr *currentInstruction = scheduleTable->linearInstructions.head;
        wtRow row;
        for (; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction) {
            row.clearEntries();
            row.entries[0].instruction = currentInstruction;
            int emulationResult = emuInst(program, &row, -1, true, true);
            row.entries[0].instruction = 0;
            if (emulationResult < 0) {
                reportError("failed to uta emu instruction\n");
                currentInstruction->dumpOperandDiagnostics(false);
                goto fail;
            }
        }
        break;
    }
    default:
        const char *modeName = codeModeName(scheduleTable->codeModeValue);
        reportError("emu - can't emu table from %s of type %s\n",
                      scheduleTable->ownerName(), modeName);
        break;
    }
    return true;
}

static inline void reportRegRefFailure()
{
    reportError("failed to add reg ref\n");
}

bool emulateAndCollectRegisterUses(Program *program, instStream *stream)
{
    int count;
    Instr *currentInstruction;
    workTable *scheduleTable;

    scheduleTable = stream->getTable();
    int linear = scheduleTable->instructionCountsByPipe[2];
    int iCount = scheduleTable->linearInstructions.count;
    if (linear != iCount) {
        reportError("linear count not same as iCount %d!=%d\n", linear, iCount);
        return false;
    }
    if (!emuTable(program, scheduleTable)) {
        reportError("Processor : failed to emu itable from '%s'\n", stream->streamName());
        return false;
    }
    count = 0;
    for (currentInstruction = scheduleTable->linearInstructions.head; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction) {
        count++;
        for (int operandIndex = 0; operandIndex != currentInstruction->operandCount; operandIndex++) {
            Operand *currentOperand = &currentInstruction->operands[operandIndex];
            RegPoolEntry *reg = currentOperand->assignedRegister;
            if (!reg)
                continue;
            regUse *registerUse = scheduleTable->registerUses.findByRegister(reg);
            if (!registerUse) {
                registerUse = scheduleTable->registerUses.AddNewRegRef(reg);
                if (!registerUse) {
                    reportRegRefFailure();
                    return false;
                }
            }
            if (isRegisterOperand(currentOperand, 2))
                registerUse->outputMask |= currentOperand->componentMask;
            if (isRegisterOperand(currentOperand, 1))
                registerUse->inputMask |= currentOperand->componentMask;
            if (isRegisterOperand(currentOperand, 2)) {
                if (currentOperand->externalPartMask) {
                    reportError("operand had xbits set for output!\n");
                    return false;
                }
            }
            registerUse->externalInputMask |= currentOperand->externalPartMask;
            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                if ((currentOperand->externalPartMask >> partIndex) & 1) {
                    if (registerUse->externalBlockMode == -1)
                        registerUse->externalBlockMode = currentOperand->externalBlockModeFlag;
                    else if (registerUse->externalBlockMode != (unsigned char)currentOperand->externalBlockModeFlag)
                        reportWarning("differing block modes for external input for %s..taking first encountered\n",
                                      reg->registerName());
                    if (!registerUse->slots[partIndex].inputLocations.addOperandLocation(currentInstruction, operandIndex, partIndex, false)) {
                        reportError("failed to add oploc user!\n");
                        return false;
                    }
                }
                if (((currentOperand->componentMask >> partIndex) & 1) && reg->tracksHotOutputPaths && isRegisterOperand(currentOperand, 2)) {
                    if (!registerUse->slots[partIndex].sourceLocations.addOperandLocation(currentInstruction, operandIndex, partIndex, false)) {
                        reportError("failed to add a local setter for a thruput check\n");
                        reportError("register is %s\n", reg->registerName());
                        return false;
                    }
                }
            }
        }
    }
    if (count != scheduleTable->instructionCountsByPipe[2]) {
        reportError("icount mismatch (%d apparently in list, but %d in table?)\n", count, scheduleTable->instructionCountsByPipe[2]);
        return false;
    }
    for (regUse *currentRegisterUse = scheduleTable->registerUses.head; currentRegisterUse; currentRegisterUse = currentRegisterUse->next) {
        RegPoolEntry *reg = currentRegisterUse->registerEntry;
        if (!reg) {
            reportError("a reg ref had no register!\n");
            return false;
        }
        for (int part = 0; part != reg->partCount; part++) {
            regUseSlot *slot = &currentRegisterUse->slots[part];
            rPart *registerPart = &reg->parts[part];
            slot->timingValues[0] = registerPart->emulationTimingMetrics[4];
            slot->timingValues[1] = registerPart->emulationTimingMetrics[5];
            slot->timingValues[2] = registerPart->emulationTimingMetrics[6];
            slot->timingValues[3] = registerPart->emulationTimingMetrics[7];
            slot->timingValues[4] = registerPart->emulationTimingMetrics[8];
            slot->timingValues[5] = registerPart->emulationTimingMetrics[9];
            slot->timingValues[6] = registerPart->emulationTimingMetrics[10];
            slot->timingValues[7] = registerPart->emulationTimingMetrics[11];
            slot->timingValues[8] = registerPart->emulationTimingMetrics[12];
            slot->timingValues[9] = registerPart->emulationTimingMetrics[13];
            slot->timingValues[10] = registerPart->emulationTimingMetrics[14];
            slot->timingValues[11] = registerPart->emulationTimingMetrics[15];
            slot->timingValues[12] = registerPart->emulationTimingMetrics[16];
            slot->throughputSums[1] = registerPart->emulationTimingMetrics[1];
            slot->throughputSums[0] = registerPart->emulationTimingMetrics[0];
            slot->throughputSums[3] = registerPart->emulationTimingMetrics[2];
            slot->throughputSums[2] = registerPart->emulationTimingMetrics[3];
            if ((currentRegisterUse->outputMask >> part) & 1) {
                ol_s *out = &registerPart->currentLocation;
                if (out->instruction && !slot->addOutput(out)) {
                    reportError("failed to add an output to an rrp\n");
                    return false;
                }
                for (int j = 0; j != 4; j++) {
                    ol_s *location = registerPart->getPendingLocation(j);
                    if (!location) {
                        reportError("failed to get a pool entry\n");
                        return false;
                    }
                    if (location->cycleOffset && !slot->addOutput(location)) {
                        reportError("failed to add a pending output to an rrp\n");
                        return false;
                    }
                }
                if (slot->outputLocations.count <= 0) {
                    reportError("no or -ve outputs during out build..nOut=%d!\n", slot->outputLocations.count);
                    return false;
                }
            }
        }
    }
    return true;
}
