#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *g_080de6ac;
extern mValue *g_080de6b0;
extern int g_080de6b4;
int func_0806aa54(Instr *in);
int func_08067e18(Program *p, instStream *s);


extern const char propagateValidMaskError[];
extern const char propagateValidDetailError[];
extern const char propagateTwoWritesError[];
extern const char propagateNormaliseError[];

bool func_0806ae58(Program *p)
{
    return true;
}

int propagateMoves(workTable *t)
{
    wtInstrList *list = &t->unk_88;
    int count = 0;
    for (Instr *in = list->tail; in; in = in->unk_1c.prev()) {
        if (in->func_080532d8() < 0) {
            reportError(propagateNormaliseError);
            return 0;
        }
        if (!in->unk_142c)
            continue;
        int mask = in->operands[0].unk_07;
        RegPoolEntry *dst = in->operands[0].unk_18;
        RegPoolEntry *src = in->operands[1].unk_18;
        int keep = 0;
        int srcWritten = 0;
        int valid = mask;
        int row = in->unk_13e8.unk_00;
        Instr *n = in->unk_1c.next();
        Instr *cur = n;
        if (row >= 0) {
            Instr *p = in->unk_1c.prev();
            if (p && p->unk_13e8.unk_00 == row) {
                bool a;
                if (p->func_08054c24(dst, &a)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool b;
                srcWritten = p->func_08054c24(src, &b);
            }
            if (n && n->unk_13e8.unk_00 == row) {
                bool a;
                if (n->func_08054c24(dst, &a)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool b;
                srcWritten |= n->func_08054c24(src, &b);
                cur = n->unk_1c.next();
            }
        }
        int pendSrc = 0;
        int pendDst = 0;
        while (cur && valid) {
            int r = cur->unk_13e8.unk_00;
            if (r < 0 || r != row) {
                srcWritten |= pendSrc;
                valid &= ~pendDst;
                pendSrc = 0;
                pendDst = 0;
            }
            bool paired = false;
            int used = cur->func_08054cc4(dst, &paired);
            if (used) {
                if (!(used & ~valid) && !(used & srcWritten) && !paired)
                    for (int i = 0; i != cur->operandCount; i++) {
                        Operand *op = &cur->operands[i];
                        if (registerOperand(op, 1) == dst)
                            op->getRegFrom(&in->operands[1]);
                        count++;
                    }
                else
                    keep |= used & valid;
            }
            bool c;
            pendSrc |= cur->func_08054c24(src, &c);
            bool d;
            pendDst |= cur->func_08054c24(dst, &d);
            cur = cur->unk_1c.unk_00;
            row = r;
        }
        if (~valid & in->operands[0].unk_27) {
            reportError(propagateValidMaskError,
                          dst->registerName(), src->registerName());
            reportError(propagateValidDetailError,
                          valid, in->operands[0].unk_27);
            return -1;
        }
        keep |= in->operands[0].unk_27;
        if (!keep)
            in->operands[0].unk_18 = in->operands[1].unk_18;
        in->unk_138f = keep;
        in->operands[0].unk_07 = keep;
        in->operands[1].unk_07 = keep;
    }
    return count;
}

int removeDeadMoves(workTable *t)
{
    int count = 0;
    Instr *next;
    for (Instr *in = t->unk_88.head; in; in = next) {
        next = in->unk_1c.unk_00;
        if (in->unk_142c && in->operands[0].unk_18 == in->operands[1].unk_18) {
            if (!t->destroyInstruction(in)) {
                reportError("failed to destroy a move\n");
                return -1;
            }
            count++;
        }
    }
    return count;
}
