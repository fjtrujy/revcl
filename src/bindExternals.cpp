#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"

const char *pipeNames[16] = {
    "VPU_FMAC", "VPU_IALU", "VPU_MOVE", "VPU_LS", "VPU_EFU",
    "VPU_DIV", "VPU_RAND", "VPU_EXT", "VPU_BRANCH", "0",
};

bool bindExternals(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("no worktable!\n");
            func_0804adf0(-1);
        }
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                RegPoolEntry *reg = op->unk_18;
                if (!reg)
                    continue;
                regUse *ru = wt->unk_14.func_0805b5dc(reg);
                if (!ru) {
                    func_0804ad38("failed to find regref in bind externals\n");
                    return false;
                }
                if (ru->isExternal() || wt->unk_08 == 3) {
                    if (reg->unk_14) {
                        func_0804ad38("found a virtual register on an x operand\n");
                        return false;
                    }
                    if (op->unk_10 && op->unk_10 != reg) {
                        func_0804ad38("operand is forced to base, but not this one\n");
                        return false;
                    }
                    op->unk_10 = reg;
                }
            }
        }
    }
    return true;
}
