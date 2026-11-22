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
        workTable *wt = stream->getTable();
        if (!wt) {
            reportError("no worktable!\n");
            exitAfterCleanup(-1);
        }
        for (Instr *instruction = wt->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int i = 0; i != instruction->operandCount; i++) {
                Operand *operand = &instruction->operands[i];
                RegPoolEntry *reg = operand->assignedRegister;
                if (!reg)
                    continue;
                regUse *registerUse = wt->registerUses.findByRegister(reg);
                if (!registerUse) {
                    reportError(bindExtFindRegRefError);
                    return false;
                }
                if (registerUse->isExternal() || wt->allocationMode == 3) {
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
