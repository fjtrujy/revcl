// Compiler-state reconstruction: config/matching-evidence/undo-move-reconstruction.md.

#include "iInfo_shared.h"
#include "Instr.h"
#include "Block.h"
#include "os.h"
void adjustMoveCount(Block *, Instr *, int);
bool addFreeMove(Instr *);
#include <stdio.h>
#include <stdarg.h>
extern bool flag_080b3038;
extern int errorOccurred;

extern const char undoMoveInputLockError[];
extern const char undoMoveParentError[];
extern const char undoMoveListError[];
extern const char undoMoveOutputRegisterError[];
extern const char undoMoveOutputDeregisterError[];
extern const char undoMoveInputRegisterError[];
extern const char undoMoveInputDeregisterError[];
extern const char undoMoveUnlinkError[];
int func_0804ad38(const char *fmt, ...)
{
    if (flag_080b3038)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    errorOccurred = 1;
    fprintf(stderr, "ERROR: ");
    return vfprintf(stderr, fmt, ap);
}

int unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
void func_08073bb0(Block *blk, Instr *mv);

bool undoMove(Block *blk, Instr *mv)
{
    if (mv->unk_142e)
        unresolveMove(blk, mv);
    Operand *out = mv->getOp(0);
    Operand *in = mv->getOp(1);
    RegPoolEntry *orig = in->unk_18;
    RegPoolEntry *dup = out->unk_18;
    if (!UnlinkFromToDo(blk, mv))
        func_0804ad38(undoMoveUnlinkError);
    for (Instr *i = blk->unk_94; i; i = (Instr *)i->unk_24.unk_00) {
        for (int n = 0; n != i->unk_138b; n++) {
            Operand *op = &i->unk_38[n];
            RegPoolEntry *r = op->unk_18;
            if (r == dup) {
                op->unk_18 = orig;
                if (isRegisterOperand(op, 1)) {
                    for (int p = 0; p != r->unk_28; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            cls0805bcc4 *lk = &op->unk_2c[p];
                            cls0805bcc4 *parent = lk->getParent();
                            cls0805c244 *newList = (cls0805c244 *)orig->func_0805a908(p);
                            cls0805c244 *oldList = (cls0805c244 *)dup->func_0805a908(p);
                            if ((bool)((out->unk_07 >> p) & 1) && parent == &out->unk_2c[p])
                                parent = in->unk_2c[p].getParent();
                            if (!oldList->removeInput(lk)) {
                                func_0804ad38(undoMoveInputDeregisterError);
                                return true;
                            }
                            if (!newList->addInput(parent, lk, op->unk_29, op->unk_0b)) {
                                func_0804ad38(undoMoveInputRegisterError);
                                return true;
                            }
                        }
                    }
                }
                if (isRegisterOperand(op, 2)) {
                    for (int p = 0; p != r->unk_28; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            cls0805c244 *newList = (cls0805c244 *)orig->func_0805a908(p);
                            cls0805c244 *oldList = (cls0805c244 *)dup->func_0805a908(p);
                            if (!oldList->removeOutput(&op->unk_2c[p])) {
                                func_0804ad38(undoMoveOutputDeregisterError);
                                return true;
                            }
                            if (!newList->addOutput(&op->unk_2c[p], i->unk_13cf, op->unk_0a, op->unk_0b)) {
                                func_0804ad38(undoMoveOutputRegisterError);
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    for (int p = 0; p != dup->unk_28; p++) {
        if ((bool)((in->unk_07 >> p) & 1)) {
            cls0805c244 *l = (cls0805c244 *)orig->func_0805a908(p);
            if (!l) {
                func_0804ad38(undoMoveListError);
                func_0804aeb0();
            }
            if (!in->unk_2c[p].getParent()) {
                func_0804ad38(undoMoveParentError);
                func_0804aeb0();
            }
            if (!l->removeInput(&in->unk_2c[p])) {
                func_0804ad38(undoMoveInputLockError);
                func_0804aeb0();
            }
        }
    }
    if (mv->unk_138b != 2) {
        func_0804ad38("move with >2 operands\n");
        func_0804aeb0();
    }
    func_08073bb0(blk, mv);
    adjustMoveCount(blk, mv, -1);
    addFreeMove(mv);
    return true;
}

