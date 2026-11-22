#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *collapseImmediateSourceRegister;
extern mValue *collapseImmediateValue;
extern int collapseImmediateOffset;
int decodeCollapsibleMoveImmediate(Instr *in);
int expandOneLoopIntoRegions(Program *p, instStream *s);


extern const char propagateValidMaskError[];
extern const char propagateValidDetailError[];
extern const char propagateTwoWritesError[];
extern const char propagateNormaliseError[];

bool noopMovePropagationHook(Program *p)
{
    return true;
}

int propagateMoves(workTable *t)
{
    wtInstrList *list = &t->linearInstructions;
    int count = 0;
    for (Instr *in = list->tail; in; in = in->linearLink.prev()) {
        if (in->normalizeZeroSources() < 0) {
            reportError(propagateNormaliseError);
            return 0;
        }
        if (!in->abstractMoveOpcode)
            continue;
        int mask = in->operands[0].componentMask;
        RegPoolEntry *dst = in->operands[0].assignedRegister;
        RegPoolEntry *src = in->operands[1].assignedRegister;
        int keep = 0;
        int srcWritten = 0;
        int valid = mask;
        int row = in->position.linearRow;
        Instr *n = in->linearLink.next();
        Instr *cur = n;
        if (row >= 0) {
            Instr *p = in->linearLink.prev();
            if (p && p->position.linearRow == row) {
                bool a;
                if (p->collectOutputRegisterMask(dst, &a)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool b;
                srcWritten = p->collectOutputRegisterMask(src, &b);
            }
            if (n && n->position.linearRow == row) {
                bool a;
                if (n->collectOutputRegisterMask(dst, &a)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool b;
                srcWritten |= n->collectOutputRegisterMask(src, &b);
                cur = n->linearLink.next();
            }
        }
        int pendSrc = 0;
        int pendDst = 0;
        while (cur && valid) {
            int r = cur->position.linearRow;
            if (r < 0 || r != row) {
                srcWritten |= pendSrc;
                valid &= ~pendDst;
                pendSrc = 0;
                pendDst = 0;
            }
            bool paired = false;
            int used = cur->collectInputRegisterMask(dst, &paired);
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
            pendSrc |= cur->collectOutputRegisterMask(src, &c);
            bool d;
            pendDst |= cur->collectOutputRegisterMask(dst, &d);
            cur = cur->linearLink.nextInstruction;
            row = r;
        }
        if (~valid & in->operands[0].externalPartMask) {
            reportError(propagateValidMaskError,
                          dst->registerName(), src->registerName());
            reportError(propagateValidDetailError,
                          valid, in->operands[0].externalPartMask);
            return -1;
        }
        keep |= in->operands[0].externalPartMask;
        if (!keep)
            in->operands[0].assignedRegister = in->operands[1].assignedRegister;
        in->destinationMask = keep;
        in->operands[0].componentMask = keep;
        in->operands[1].componentMask = keep;
    }
    return count;
}

int removeDeadMoves(workTable *t)
{
    int count = 0;
    Instr *next;
    for (Instr *in = t->linearInstructions.head; in; in = next) {
        next = in->linearLink.nextInstruction;
        if (in->abstractMoveOpcode && in->operands[0].assignedRegister == in->operands[1].assignedRegister) {
            if (!t->destroyInstruction(in)) {
                reportError("failed to destroy a move\n");
                return -1;
            }
            count++;
        }
    }
    return count;
}
