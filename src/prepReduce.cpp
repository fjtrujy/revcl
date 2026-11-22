#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "instStream.h"

bool prepareReductionCandidates(Program *program, instStream *stream, workTable *scheduleTable)
{
    if (stream->usesFreeRawLayout()) {
        stream->allowUpwardReduction = 1;
        stream->allowDownwardReduction = 1;
        stream->reductionSkipped = 0;
        bool ext = false;
        workTable *reductionTable = stream->getTable();
        int len = reductionTable->getRangeEnd() - reductionTable->getRangeStart();
        if (stream->participatesInStall || len < reductionTable->externalMinimumLength) {
            reductionTable->setTableRawMode(2);
            stream->allowUpwardReduction = 0;
            stream->allowDownwardReduction = 0;
            stream->reductionSkipped = 1;
            reportInfo("table %s appears to have stalls - so skipping reduce pass\n",
                          stream->streamName());
            return true;
        }
        regUseList *registerUseList = &scheduleTable->registerUses;
        for (regUse *registerUse = registerUseList->head; registerUse; registerUse = registerUse->next)
            if (registerUse->hazardBroadcastMask)
                ext = true;
        if (ext) {
            stream->allowUpwardReduction = 0;
            stream->allowDownwardReduction = 0;
            stream->reductionSkipped = 1;
        }
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                RegPoolEntry *reg = operand->assignedRegister;
                operand->reductionHazardMask = 0;
                if (!reg)
                    continue;
                regUse *currentRegisterUse = registerUseList->findByRegister(reg);
                if (isRegisterOperand(operand, 1))
                    operand->reductionHazardMask = currentRegisterUse->hazardMask & operand->externalPartMask;
                if (isRegisterOperand(operand, 2))
                    operand->reductionHazardMask = currentRegisterUse->sourcePathHazardMask & operand->externalPartMask;
                operand->reductionHazardMask |= currentRegisterUse->hazardBroadcastMask;
            }
        }
    }
    return true;
}

extern const char linearReduceTakeOutError[] = "failed to take out instruction\n";
