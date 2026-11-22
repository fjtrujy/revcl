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
        workTable *t = stream->getTable();
        int len = t->getRangeEnd() - t->getRangeStart();
        if (stream->participatesInStall || len < t->externalMinimumLength) {
            t->setTableRawMode(2);
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
                regUse *ru = registerUseList->findByRegister(reg);
                if (isRegisterOperand(operand, 1))
                    operand->reductionHazardMask = ru->hazardMask & operand->externalPartMask;
                if (isRegisterOperand(operand, 2))
                    operand->reductionHazardMask = ru->sourcePathHazardMask & operand->externalPartMask;
                operand->reductionHazardMask |= ru->hazardBroadcastMask;
            }
        }
    }
    return true;
}

extern const char linearReduceTakeOutError[] = "failed to take out instruction\n";
