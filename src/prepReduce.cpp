#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "instStream.h"

bool prepareReductionCandidates(Program *p, instStream *s, workTable *wt)
{
    if (s->usesFreeRawLayout()) {
        s->unk_261 = 1;
        s->unk_262 = 1;
        s->unk_263 = 0;
        bool ext = false;
        workTable *t = s->getTable();
        int len = t->getRangeEnd() - t->getRangeStart();
        if (s->participatesInStall || len < t->externalMinimumLength) {
            t->setTableRawMode(2);
            s->unk_261 = 0;
            s->unk_262 = 0;
            s->unk_263 = 1;
            reportInfo("table %s appears to have stalls - so skipping reduce pass\n",
                          s->streamName());
            return true;
        }
        regUseList *ul = &wt->registerUses;
        for (regUse *r = ul->head; r; r = r->next)
            if (r->hazardBroadcastMask)
                ext = true;
        if (ext) {
            s->unk_261 = 0;
            s->unk_262 = 0;
            s->unk_263 = 1;
        }
        for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
            for (int i = 0; i != in->operandCount; i++) {
                Operand *op = &in->operands[i];
                RegPoolEntry *reg = op->assignedRegister;
                op->unk_28 = 0;
                if (!reg)
                    continue;
                regUse *ru = ul->findByRegister(reg);
                if (isRegisterOperand(op, 1))
                    op->unk_28 = ru->hazardMask & op->externalPartMask;
                if (isRegisterOperand(op, 2))
                    op->unk_28 = ru->unk_13 & op->externalPartMask;
                op->unk_28 |= ru->hazardBroadcastMask;
            }
        }
    }
    return true;
}

extern const char linearReduceTakeOutError[] = "failed to take out instruction\n";
