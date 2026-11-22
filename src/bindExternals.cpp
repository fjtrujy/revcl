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

bool bindExternals(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        if (!wt) {
            reportError("no worktable!\n");
            exitAfterCleanup(-1);
        }
        for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
            for (int i = 0; i != in->operandCount; i++) {
                Operand *op = &in->operands[i];
                RegPoolEntry *reg = op->assignedRegister;
                if (!reg)
                    continue;
                regUse *ru = wt->registerUses.findByRegister(reg);
                if (!ru) {
                    reportError(bindExtFindRegRefError);
                    return false;
                }
                if (ru->isExternal() || wt->allocationMode == 3) {
                    if (reg->isVirtual) {
                        reportError(bindExtVirtualRegError);
                        return false;
                    }
                    if (op->forcedBaseRegister && op->forcedBaseRegister != reg) {
                        reportError(bindExtForcedBaseError);
                        return false;
                    }
                    op->forcedBaseRegister = reg;
                }
            }
        }
    }
    return true;
}
