#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

extern const char lpWorkSavedTableError[];
extern const char lpWorkHtNewError[];
extern const char lpWorkCloneTableError[];
extern const char lpWorkNoTableError[];
extern const char lpWorkDisposeNoTableError[];
extern const char lpWorkDisposeNoSavedError[];
extern const char lsqChainHopeError[];
extern const char lsqChainLinkError[];
extern const char lsqChainNotInputError[];
extern const char lsqChainAddError[];
extern const char lsqChainNewInstrError[];


bool makeLpWork(instStream *stream, int maxSpread)
{
    workTable *table = stream->workTablePtr;
    if (!table) {
        reportError(lpWorkNoTableError);
        return false;
    }
    if (stream->savedTable) {
        reportError(lpWorkSavedTableError);
        return false;
    }
    stream->savedTable = table;
    stream->workTablePtr = 0;
    workTable *scheduleTable = table->localClone();
    stream->workTablePtr = scheduleTable;
    if (!scheduleTable) {
        reportError(lpWorkCloneTableError);
        return false;
    }
    Instr *instruction;
    for (instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        int latest = 0;
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegPoolEntry *registerEntry = registerOperand(operand, 1);
            if (registerEntry) {
                for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                    ol_s *operandLocation = operand->parts[partIndex].operandLocations.head;
                    if (operandLocation) {
                        Instr *sourceInstruction = operandLocation->instruction;
                        Operand *otherOperand = operandAtLocation(operandLocation);
                        if (!sourceInstruction || !otherOperand) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int latestRequiredRow = sourceInstruction->instructionId + otherOperand->outputLatency + operand->additionalLatency;
                        if (latest < latestRequiredRow)
                            latest = latestRequiredRow;
                    }
                }
            }
        }
        instruction->instructionId = latest;
    }
    for (instruction = scheduleTable->linearInstructions.tail; instruction; instruction = instruction->linearLink.previousInstruction) {
        int earliest = 30000;
        for (int sourceOperandIndex = 0; sourceOperandIndex != instruction->operandCount; sourceOperandIndex++) {
            Operand *operand = &instruction->operands[sourceOperandIndex];
            RegPoolEntry *registerEntry = registerOperand(operand, 2);
            if (registerEntry) {
                for (int sourcePartIndex = 0; sourcePartIndex != registerEntry->partCount; sourcePartIndex++) {
                    for (ol_s *operandLocation = operand->parts[sourcePartIndex].operandLocations.head; operandLocation; operandLocation = operandLocation->next) {
                        Instr *destinationInstruction = operandLocation->instruction;
                        Operand *otherOperand = operandAtLocation(operandLocation);
                        if (!destinationInstruction || !otherOperand) {
                            reportError("in_s not set\n");
                            return false;
                        }
                        int earliestRequiredRow = destinationInstruction->instructionId - operand->outputLatency - otherOperand->additionalLatency;
                        if (earliest > earliestRequiredRow)
                            earliest = earliestRequiredRow;
                    }
                }
            }
        }
        if (earliest != 30000) {
            if (earliest < instruction->instructionId) {
                reportError(lpWorkHtNewError);
                return false;
            }
            instruction->instructionId = earliest;
        }
    }
    char buf[256];
    for (instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        int minimumDependencyRow = 30000;
        int maximumDependencyRow = 0;
        char *outputCursor = buf;
        for (int destinationOperandIndex = 0; destinationOperandIndex != instruction->operandCount; destinationOperandIndex++) {
            Operand *operand = &instruction->operands[destinationOperandIndex];
            RegPoolEntry *registerEntry = registerOperand(operand, 1);
            if (registerEntry) {
                outputCursor += formatIntoBuffer(outputCursor, " %s: ", registerEntry->registerName());
                int rlo = 30000, rhi = 0;
                bool any = false;
                for (int destinationPartIndex = 0; destinationPartIndex != registerEntry->partCount; destinationPartIndex++) {
                    if ((operand->componentMask >> destinationPartIndex) & 1) {
                        ol_s *operandLocation = operand->parts[destinationPartIndex].operandLocations.head;
                        if (operandLocation) {
                            Instr *sourceInstruction = operandLocation->instruction;
                            int dependencyDistance = instruction->instructionId - sourceInstruction->instructionId;
                            any = true;
                            if (dependencyDistance < rlo)
                                rlo = dependencyDistance;
                            if (dependencyDistance > rhi)
                                rhi = dependencyDistance;
                            outputCursor += formatIntoBuffer(outputCursor, "%3d ", dependencyDistance);
                        } else
                            outputCursor += formatIntoBuffer(outputCursor, "xxx ");
                    } else
                        outputCursor += formatIntoBuffer(outputCursor, "... ");
                }
                if (any) {
                    outputCursor += formatIntoBuffer(outputCursor, "[%d %d] (%d) ", rlo, rhi, rhi - rlo);
                    if (rhi - rlo > maxSpread)
                        outputCursor += formatIntoBuffer(outputCursor, "!!!");
                    if (rlo < minimumDependencyRow)
                        minimumDependencyRow = rlo;
                    if (rhi > maximumDependencyRow)
                        maximumDependencyRow = rhi;
                }
            }
        }
        outputCursor += formatIntoBuffer(outputCursor, "    -> %d %d (%d)", minimumDependencyRow, maximumDependencyRow, maximumDependencyRow - minimumDependencyRow);
        if (minimumDependencyRow > maxSpread)
            outputCursor += formatIntoBuffer(outputCursor, " ****");
        outputCursor[10] = 0;
    }
    return true;
}

bool disposeLpWork(instStream *stream)
{
    if (stream->workTablePtr == 0) {
        reportError(lpWorkDisposeNoTableError);
        return false;
    }
    delete stream->workTablePtr;
    if (stream->savedTable == 0) {
        reportError(lpWorkDisposeNoSavedError);
        return false;
    }
    stream->workTablePtr = stream->savedTable;
    stream->savedTable = 0;
    return true;
}

bool keepLpWork(instStream *stream)
{
    stream->savedTable = 0;
    return true;
}

bool modLsqq(Instr *in, int imm);

bool lsqChain(workTable *table)
{
    reportError(lsqChainHopeError);
    abortAfterError();
    for (regUse *registerUse = table->registerUses.head; registerUse; registerUse = registerUse->next) {
        RegPoolEntry *registerEntry = registerUse->registerEntry;
        if (registerEntry->registerClass != 8)
            continue;
        Instr *list[1024];
        Instr *instruction = table->linearInstructions.head;
        reportInfo("lsqi- scan %s\n", registerEntry->registerName());
        int chainedInstructionCount = 0;
        bool bad = false;
        bool ok = true;
        for (; instruction; instruction = instruction->linearLink.nextInstruction) {
            int opc = instruction->opcodeId();
            if (instruction->outputMaskForRegister(registerEntry, 0xff)) {
                if (!(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4) || bad) {
                    ok = false;
                    break;
                }
                list[chainedInstructionCount++] = instruction;
            }
            if (instruction->inputMaskForRegister(registerEntry, 0xff)
                && !(opc == 0x9e || opc == 0x9f || opc == 0xa3 || opc == 0xa4))
                bad = true;
        }
        if (!ok || chainedInstructionCount <= 1)
            continue;
        reportInfo("lsChain for %s\n", registerEntry->registerName());
        Operand *sourceOperand = &list[0]->operands[1];
        if (sourceOperand->externalPartMask != 1) {
            reportError("this isn't externally set!\n");
            abortAfterError();
        }
        int total = 0;
        for (int instructionIndex = 0; instructionIndex != chainedInstructionCount; instructionIndex++) {
            Instr *instruction = list[instructionIndex];
            int opc = instruction->opcodeId();
            int delta = 0;
            switch (opc) {
            case 0x9e:
            case 0xa3:
                delta = 1;
                break;
            case 0x9f:
            case 0xa4:
                delta = -1;
                break;
            }
            if (!modLsqq(instruction, total))
                reportError("mod instruction\n");
            total += delta;
            const char *name = "NO_BASE_INST";
            if (instruction->opcodeInfo)
                name = instruction->opcodeInfo->mnemonic;
            reportInfo("  %3d : %s\n", instructionIndex, name);
        }
        iInfo *opcodeInfo;
        if (total < 0) {
            opcodeInfo = getOpcodeInfoById(0x70);
            total = -total;
        } else
            opcodeInfo = getOpcodeInfoById(0x6e);
        Instr *newInstruction = new Instr(opcodeInfo);
        if (!newInstruction) {
            reportError(lsqChainNewInstrError);
            return false;
        }
        newInstruction->operands[0].assignedRegister = registerEntry;
        newInstruction->operands[0].externalPartMask = 1;
        newInstruction->operands[0].componentMask = 1;
        newInstruction->operands[1].assignedRegister = registerEntry;
        newInstruction->operands[1].externalPartMask = 1;
        newInstruction->operands[1].componentMask = 1;
        newInstruction->operands[2].immediateValue.setInt(total);
        if (!table->logAddLinear_After(list[chainedInstructionCount - 1], newInstruction)) {
            reportError(lsqChainAddError);
            return false;
        }
        for (Instr *instruction = newInstruction->linearLink.nextInstruction; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                if (operand->assignedRegister != registerEntry)
                    continue;
                if (!isRegisterOperand(operand, 1)) {
                    reportError(lsqChainNotInputError);
                    return false;
                }
                operand->parts[0].operandLocations.clear();
                if (!linkOperandParts(newInstruction, 0, 0, instruction, operandIndex, 0)) {
                    reportError(lsqChainLinkError);
                    return false;
                }
            }
        }
    }
    return true;
}

bool splitStreams()
{
    reportError("split streams called\n");
    return false;
}

bool modLsqq(Instr *instruction, int imm)
{
    iInfo *opcodeInfo;
    switch ((unsigned char)instruction->ramAccessType) {
    case 1:
        opcodeInfo = getOpcodeInfoById(0x9b);
        break;
    case 2:
        opcodeInfo = getOpcodeInfoById(0xa0);
        break;
    default:
        reportError("mod_lsqq: ?\n");
        abortAfterError();
        return false;
    }
    instruction->opcodeInfo = opcodeInfo;
    instruction->operands[2].parts[0].clearSlotLocations();
    instruction->operands[1].parts[0].clearSlotLocations();
    instruction->operandCount = 2;
    instruction->operands[1].externalPartMask = 1;
    Operand *operand = &instruction->operands[1];
    iOpDesc *descriptor = &opcodeInfo->operandDescriptors[1];
    operand->operandKind = descriptor->operandKind;
    operand->isImplicit = descriptor->isImplicit != 0;
    operand->immediateDataType = descriptor->immediateDataType;
    operand->pairedOperandIndexPlusOne = descriptor->patchedOutputIndexPlusOne;
    if (operand->operandKind != 0x1d) {
        reportError("modded lqsq isn't ea_imm_reg\n");
        abortAfterError();
        return false;
    }
    if (!operand->immediateValue.setInt(imm)) {
        reportError("failed to set new immediate\n");
        return false;
    }
    return true;
}
