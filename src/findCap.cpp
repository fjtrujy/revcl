#include "sred_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int countSetBits(unsigned int x);
RegPoolEntry *getPoolZeroRegister(unsigned int type);


static inline bool isOut(Operand *operand) { return operand->direction == 2; }
static inline RegPoolEntry *outR(Operand *operand)
{
    if (isOut(operand))
        return operand->assignedRegister;
    return 0;
}
static inline bool isPoolZero(RegPool *registerPool, RegPoolEntry *registerEntry) { return registerEntry == registerPool->zeroRegister; }

int findCap(workTable *scheduleTable, Instr *instruction, int opIdx, RegPoolEntry **capReg, int *capMask)
{
    capInsertionWorkTable = scheduleTable;
    selectedCapRegister = 0;
    *capReg = 0;
    Operand *operand = &instruction->operands[opIdx];
    int cap = -1;
    selectedCapPipelineProperties = 0;
    selectedCapMask = 0;
    *capMask = 0;
    selectedCapOpcodeInfo = 0;
    selectedCapOriginalInstruction = 0;

    RegPoolEntry *registerEntry = isRegisterOperand(operand, 2) ? operand->assignedRegister : 0;
    switch (registerEntry->registerIndex) {
    case 52:
        cap = 0x73;
        selectedCapRegister = findRegisterByGlobalIndex(51);
        selectedCapMask = 1;
        break;
    case 50:
        selectedCapRegister = registerEntry;
        selectedCapMask = 1;
        cap = 0x74;
        break;
    case 55: case 57: case 59: {
        if (instruction->position.getPipe())
            break;
        RegPoolEntry *r0 = outR(&instruction->operands[0]);
        if (!r0) {
            const char *name = instruction->opcodeInfo ? instruction->opcodeInfo->mnemonic : "NO_BASE_INST";
            reportError("couldn't get r0 for %s\n", name);
            return -1;
        }
        if (r0->registerClass != 9)
            break;
        RegPool *pool = r0->ownerPool;
        if (isPoolZero(pool, r0)) {
            unsigned char mask[32];
            int registerIndex;
            for (registerIndex = 0; registerIndex != 32; registerIndex++) {
                RegPoolEntry *candidateRegister = pool->findByRegisterIndex(registerIndex);
                mask[registerIndex] = (candidateRegister->enabledPhysicalRegister && candidateRegister->allocatable) ? instruction->operands[0].componentMask : 0;
            }
            mask[0] = 0;
            for (regUse *registerUse = scheduleTable->registerUses.head; registerUse; registerUse = registerUse->next) {
                RegPoolEntry *registerEntry = registerUse->registerEntry;
                if (registerEntry->registerClass != 9)
                    continue;
                if (registerEntry->isVirtual == 1) {
                    reportError("register isn't base!\n");
                    return -1;
                }
                unsigned char usedComponentMask = registerUse->inputMask | registerUse->outputMask | registerUse->throughputMask;
                int idx = pool->regIndex(registerEntry->registerIndex);
                if (idx < 0) {
                    reportError("failed to get local id\n");
                    return -1;
                }
                if (usedComponentMask & mask[idx])
                    mask[idx] = 0;
            }
            int best = 0;
            for (registerIndex = 1; registerIndex != 32; registerIndex++) {
                int matchingComponentMask = instruction->operands[0].componentMask & mask[registerIndex];
                int matchedComponentCount = countSetBits(matchingComponentMask);
                if (matchedComponentCount > best) {
                    RegPoolEntry *selectedRegister = pool->findByRegisterIndex(registerIndex);
                    selectedCapMask = matchingComponentMask;
                    selectedCapRegister = selectedRegister;
                    best = matchedComponentCount;
                }
            }
        } else {
            selectedCapMask = instruction->operands[0].componentMask;
            selectedCapRegister = r0;
        }
        if (selectedCapRegister)
            cap = 0x21;
        break;
    }
    }

    if (cap < 0)
        return 0;
    iInfo *instructionInfo = getOpcodeInfoById(cap);
    selectedCapOpcodeInfo = instructionInfo;
    if (!instructionInfo && cap != 0) {
        reportError("failed to get II for cap \n");
        return -1;
    }
    if (!instructionInfo) {
        reportError("failed to get II for cap \n");
        return -1;
    }
    *capReg = selectedCapRegister;
    selectedCapOriginalInstruction = instruction;
    *capMask = selectedCapMask;
    selectedCapPipelineProperties = instructionInfo->unk_0e_w;
    return selectedCapPipelineProperties;
}

bool insertRegisterCapInstruction(int line, int pipe, int mask)
{
    if (!selectedCapOpcodeInfo) {
        reportError("cap_ii not set\n");
        return false;
    }
    if (!mask) {
        reportError("zero mod bits for doCap\n");
        return false;
    }
    Instr *capInstruction = new Instr(selectedCapOpcodeInfo);
    if (!capInstruction) {
        reportError("failed to make cap instruction\n");
        return false;
    }
    capInstruction->destinationMask = mask;
    for (int operandIndex = 0; operandIndex != capInstruction->operandCount; operandIndex++)
        capInstruction->operands[operandIndex].componentMask = mask;

    if (selectedCapRegister->registerClass == 9) {
        RegPoolEntry *registerEntry = getPoolZeroRegister(9);
        capInstruction->operands[0].assignedRegister = registerEntry;
        capInstruction->operands[0].originalRegister = registerEntry;
        capInstruction->operands[0].forcedBaseRegister = 0;
        capInstruction->operands[1].assignedRegister = selectedCapRegister;
        capInstruction->operands[1].originalRegister = selectedCapRegister;
        capInstruction->operands[1].forcedBaseRegister = 0;
        selectedCapOriginalInstruction->operands[0].originalRegister = selectedCapRegister;
        selectedCapOriginalInstruction->operands[0].assignedRegister = selectedCapRegister;
        selectedCapOriginalInstruction->operands[0].forcedBaseRegister = 0;
        regUse *registerUse = capInsertionWorkTable->registerUses.findByRegister(selectedCapRegister);
        if (!registerUse) {
            registerUse = capInsertionWorkTable->registerUses.addRegisterUse(selectedCapRegister);
            if (!registerUse) {
                reportError("failed to add new regref\n");
                return true;
            }
        }
        registerUse->inputMask |= mask;
        registerUse->outputMask |= mask;
    } else {
        capInstruction->operands[0].assignedRegister = selectedCapRegister;
        capInstruction->operands[0].originalRegister = selectedCapRegister;
    }
    capInstruction->position.setFromLinearRow(line);
    capInstruction->position.setInstructionPipe(pipe);
    if (!capInsertionWorkTable->putIn(capInstruction)) {
        reportError("failed to put instruction into itable\n");
        return false;
    }
    selectedCapOpcodeInfo = 0;
    return true;
}
