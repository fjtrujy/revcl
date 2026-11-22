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
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        if (!scheduleTable) {
            reportError("no worktable!\n");
            exitAfterCleanup(-1);
        }
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                RegPoolEntry *reg = operand->assignedRegister;
                if (!reg)
                    continue;
                regUse *registerUse = scheduleTable->registerUses.findByRegister(reg);
                if (!registerUse) {
                    reportError(bindExtFindRegRefError);
                    return false;
                }
                if (registerUse->isExternal() || scheduleTable->allocationMode == 3) {
                    if (reg->isVirtual) {
                        reportError(bindExtVirtualRegError);
                        return false;
                    }
                    if (operand->forcedBaseRegister && operand->forcedBaseRegister != reg) {
                        reportError(bindExtForcedBaseError);
                        return false;
                    }
                    operand->forcedBaseRegister = reg;
                }
            }
        }
    }
    return true;
}
