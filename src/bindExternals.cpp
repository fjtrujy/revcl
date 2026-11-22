#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "decodedLine.h"

extern const char bindExtForcedBaseError[];
extern const char bindExtVirtualRegError[];
extern const char bindExtFindRegRefError[];

bool bindExternals(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        ScheduleTable *scheduleTable = stream->getTable();
        if (!scheduleTable) {
            reportError("no worktable!\n");
            exitAfterCleanup(-1);
        }
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                RegisterEntry *registerEntry = operand->assignedRegister;
                if (!registerEntry)
                    continue;
                RegisterUse *registerUse = scheduleTable->registerUses.findByRegister(registerEntry);
                if (!registerUse) {
                    reportError(bindExtFindRegRefError);
                    return false;
                }
                if (registerUse->isExternal() || scheduleTable->allocationMode == VCL_ALLOC_DONE) {
                    if (registerEntry->isVirtual) {
                        reportError(bindExtVirtualRegError);
                        return false;
                    }
                    if (operand->forcedBaseRegister && operand->forcedBaseRegister != registerEntry) {
                        reportError(bindExtForcedBaseError);
                        return false;
                    }
                    operand->forcedBaseRegister = registerEntry;
                }
            }
        }
    }
    return true;
}
