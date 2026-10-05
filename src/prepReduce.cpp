#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "instStream.h"

bool func_0807e528(Program *p, instStream *s, workTable *wt)
{
    if (s->func_0806d9a0()) {
        s->unk_261 = 1;
        s->unk_262 = 1;
        s->unk_263 = 0;
        bool ext = false;
        workTable *t = s->getTable();
        int len = t->func_080741c0() - t->func_0807411c();
        if (s->unk_260 || len < t->unk_44) {
            t->func_08073774(2);
            s->unk_261 = 0;
            s->unk_262 = 0;
            s->unk_263 = 1;
            func_0804acb0("table %s appears to have stalls - so skipping reduce pass\n",
                          s->func_0806d674());
            return true;
        }
        regUseList *ul = &wt->unk_14;
        for (regUse *r = ul->unk_04; r; r = r->unk_04)
            if (r->unk_14)
                ext = true;
        if (ext) {
            s->unk_261 = 0;
            s->unk_262 = 0;
            s->unk_263 = 1;
        }
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                RegPoolEntry *reg = op->unk_18;
                op->unk_28 = 0;
                if (!reg)
                    continue;
                regUse *ru = ul->func_0805b5dc(reg);
                if (isRegisterOperand(op, 1))
                    op->unk_28 = ru->unk_12 & op->unk_27;
                if (isRegisterOperand(op, 2))
                    op->unk_28 = ru->unk_13 & op->unk_27;
                op->unk_28 |= ru->unk_14;
            }
        }
    }
    return true;
}
