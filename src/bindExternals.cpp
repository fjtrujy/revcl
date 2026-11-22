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
        for (Instr *in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
            for (int i = 0; i != in->operandCount; i++) {
                Operand *op = &in->operands[i];
                RegPoolEntry *reg = op->unk_18;
                if (!reg)
                    continue;
                regUse *ru = wt->unk_14.findByRegister(reg);
                if (!ru) {
                    reportError(bindExtFindRegRefError);
                    return false;
                }
                if (ru->isExternal() || wt->unk_08 == 3) {
                    if (reg->isVirtual) {
                        reportError(bindExtVirtualRegError);
                        return false;
                    }
                    if (op->unk_10 && op->unk_10 != reg) {
                        reportError(bindExtForcedBaseError);
                        return false;
                    }
                    op->unk_10 = reg;
                }
            }
        }
    }
    return true;
}
