extern const char getMoveParentUninsertedError[];
extern const char getMoveNegativeDistanceError[];
extern const char getMoveNoRegisterError[];
extern const char getMoveNoParentError[];
extern const char getMoveOldStateError[];
extern const char getMoveSpreadStateError[];
extern const char getMoveInputOwnerError[];
extern const char getMoveOutputOwnerError[];
extern const char getMoveInputRegisterError[];
extern const char getMoveOutputLockError[];
extern const char getMoveInputDeregisterError[];
extern const char getMoveOutputInputError[];
extern const char getMoveCandidateError[];
extern const char getMoveNewInputError[];
extern const char getMoveRetargetDeregisterError[];
extern const char getMoveRetargetReregisterError[];
extern const char getMoveTodoHeadError[];
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


#include "iInfo_shared.h"

#include "Instr.h"
#include "Block.h"
#include "os.h"


Instr *makeMove(Block *, RegPoolEntry *);
void adjustMoveCount(Block *blk, Instr *mv, int);
bool addFreeMove(Instr *mv);
bool setPipeBits(Instr *in);
bool AddToDo_Head(Block *blk, Instr *n);
bool logInstr(Block *blk, Instr *in);
void func_080737b8(Instr *in);

int g_080b33ec = 0;

static inline void countCreatedMove(Block *blk, Instr *mv)
{
    adjustMoveCount(blk, mv, 1);
}

int getMove(Block *blk, Instr *in, int now)
{
    Operand *bestOp = 0;
    int bestScore = -1;
    RegPoolEntry *bestReg = 0;
    cls0805bcc4 *bestLk = 0;
    if (in->unk_1428)
        return 2;
    Operand *op;
    int i;
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            r->unk_00 = 0x7fff;
            r->unk_02 = -1;
        }
    }
    Operand *op2;
    for (i = 0; (op2 = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op2, 1);
        if (reg && func_0806c6c4(reg) && op2->unk_2b != 2) {
            cls0805bcc4 *lk = 0;
            int score = -1;
            for (int p = 0; p != reg->unk_28; p++) {
                if ((op2->unk_07 >> p) & 1) {
                    lk = &op2->unk_2c[p];
                    cls0805bcc4 *parent = lk->getParent();
                    if (lk->getPriority() == 2 && score < 10000)
                        score = 10000;
                    if (lk->getPriority() == 1 && score < 20000)
                        score = 20000;
                    if (parent->unk_2d != 2) {
                        int row = parent->unk_32;
                        int z = op2->unk_07;
                        if (row < 0) {
                            z >>= 4;
                            z >>= 4;
                            if (z <= 0) {
                                func_0804ad38(getMoveParentUninsertedError);
                                func_0804aeb0();
                            }
                        }
                        int lo = row + 1;
                        int hi = row + parent->unk_18;
                        if (lo < reg->unk_00)
                            reg->unk_00 = lo;
                        if (hi > reg->unk_02)
                            reg->unk_02 = hi;
                        int d = now - lo;
                        if (d < 0) {
                            func_0804ad38(getMoveNegativeDistanceError);
                            func_0804aeb0();
                        }
                        RegPoolEntry *uA = op2->unk_10;
                        cls0805c244 *u04 = parent->unk_04;
                        if (d > score) {
                            op2->unk_10 = uA;
                            parent->unk_04 = u04;
                            score = d;
                        }
                        RegPoolEntry *u14b = op2->unk_14;
                        op2->unk_14 = u14b;
                    }
                }
            }
            int s1 = in->unk_00;
            Instr *s2 = blk->unk_94;
            if (score > bestScore) {
                in->unk_00 = s1;
                blk->unk_94 = s2;
                bestOp = op2;
                bestScore = score;
                bestReg = reg;
                bestLk = lk;
            }
        }
    }
    op = 0;
    if (!bestReg) {
        func_0804ac7c(getMoveNoRegisterError);
        return 4;
    }
    cls0805bcc4 *parent = bestLk->getParent();
    if (!parent) {
        func_0804ad38(getMoveNoParentError);
        return -1;
    }
    Instr *owner = parent->ownerInstr();
    int mask = 0;
    int p;
    for (p = 0; p != bestReg->unk_28; p++) {
        cls0805bcc4 *o = ((cls0805c244 *)bestReg->func_0805a908(p))->unk_1c;
        if (o && o->unk_22)
            mask |= 1 << p;
    }
    Instr *mv = makeMove(blk, bestReg);
    if (!mv) {
    no_move:
        Instr *earliest = 0;
        for (p = 0; p != bestReg->unk_28; p++) {
            cls0805bcc4 *o = ((cls0805c244 *)bestReg->func_0805a908(p))->unk_1c;
            if (((mask >> p) & 1) && o && o->unk_22) {
                Instr *oi = o->ownerInstr();
                if (oi && (!earliest || oi->unk_13e8.getRow() < earliest->unk_13e8.getRow()))
                    earliest = oi;
            }
        }
        if (!earliest)
            earliest = blk->unk_a0;
        func_080737b8(earliest);
        return 1;
    }
    {
        setPipeBits(mv);
        if (owner) {
            mv->unk_13e4 = owner->unk_13e4 + 1;
            mv->unk_1432 = owner->unk_1432 + 1;
            mv->unk_141a = owner->unk_141a;
            mv->unk_13f0.func_0805e37c(&owner->unk_13f0);
        } else {
            mv->unk_1432 = 1;
            mv->unk_13e4 = in->unk_13e4 - 1;
            mv->unk_141a = in->unk_141a;
            mv->unk_13f0.func_0805e37c(&in->unk_13f0);
        }
        mv->unk_141c = 0;
        Operand *out = mv->getOp(0);
        Operand *mvIn = mv->getOp(1);
        RegPoolEntry *newReg = out->unk_18;
        mvIn->unk_14 = bestOp->unk_14;
        out->unk_14 = newReg;
        short targetRow = out->unk_0a;
        mv->unk_1430 = now - targetRow;
        int moved = 0;
        int oldBits = 0;
        int newBits = 0;
        for (Instr *it = blk->unk_94; it; it = (Instr *)it->unk_24.unk_00) {
            int n;
            for (n = 0; n != it->unk_138b; n++) {
                Operand *op = &it->unk_38[n];
                RegPoolEntry *reg = registerOperand(op, 1);
                if (reg == bestReg) {
                    bool stale = false;
                    if (op->unk_2b != 1) {
                        if (op->unk_07 & newBits) {
                            func_0804ad38(getMoveOldStateError);
                            func_0804aeb0();
                        }
                        stale = true;
                    }
                    if (op->unk_07 & oldBits) {
                        if (op->unk_07 & newBits) {
                            func_0804ad38(getMoveSpreadStateError);
                            func_0804aeb0();
                        }
                        stale = true;
                    }
                    if (stale) {
                        mv->unk_13d1 = 0;
                        continue;
                    }
                    for (int p = 0; p != reg->unk_28; p++) {
                        if ((op->unk_07 >> p) & 1) {
                            cls0805bcc4 *lk = &op->unk_2c[p];
                            cls0805bcc4 *par = lk->getParent();
                            cls0805c244 *oldL = (cls0805c244 *)bestReg->func_0805a908(p);
                            cls0805c244 *newL = (cls0805c244 *)newReg->func_0805a908(p);
                            if (par->unk_2c) {
                                cls0805bcc4 *oldOut = oldL->unk_1c;
                                if (!((moved >> p) & 1)) {
                                    cls0805bcc4 *moveInLock = &mvIn->unk_2c[p];
                                    if (!moveInLock->setOwner(p, mv, 1)) {
                                        func_0804ad38(getMoveInputOwnerError);
                                        return -1;
                                    }
                                    if (!out->unk_2c[p].setOwner(p, mv, 0)) {
                                        func_0804ad38(getMoveOutputOwnerError);
                                        return -1;
                                    }
                                    cls0805bcc4 *moveOutLock = &out->unk_2c[p];
                                    if (!oldL->addInput(oldOut, moveInLock, mvIn->unk_29, mvIn->unk_0b)) {
                                        func_0804ad38(getMoveInputRegisterError);
                                        return -1;
                                    }
                                    moveInLock->unk_19 = mvIn->getStatus();
                                    if (!newL->addOutput(moveOutLock, mv->unk_13cf, out->unk_0a, out->unk_0b)) {
                                        func_0804ad38(getMoveOutputLockError);
                                        return -1;
                                    }
                                    moveOutLock->unk_19 = out->getStatus();
                                }
                                moved |= 1 << p;
                                if (!oldL->removeInput(lk)) {
                                    func_0804ad38(getMoveInputDeregisterError);
                                    return -1;
                                }
                                if (!newL->addInput(&out->unk_2c[p], lk, op->unk_29, op->unk_0b)) {
                                    func_0804ad38(getMoveOutputInputError);
                                    return -1;
                                }
                                op->unk_18 = newReg;
                            } else {
                                if (par->unk_04 == newL) {
                                    if (!oldL->removeInput(lk)) {
                                        func_0804ad38(getMoveInputDeregisterError);
                                        return -1;
                                    }
                                    if (!newL->addInput(par, lk, op->unk_29, op->unk_0b)) {
                                        func_0804ad38(getMoveNewInputError);
                                        return -1;
                                    }
                                    op->unk_18 = newReg;
                                } else {
                                    func_0804ad38(getMoveCandidateError);
                                    func_0804aeb0();
                                }
                            }
                        }
                    }
                }
            }
            for (n = 0; n != it->unk_138b; n++) {
                Operand *op = &it->unk_38[n];
                RegPoolEntry *reg = registerOperand(op, 2);
                if (reg == bestReg) {
                    if (op->unk_2b != 1) {
                        mv->unk_13d1 = 0;
                        oldBits |= op->unk_07;
                    } else {
                        newBits |= op->unk_07;
                        for (int p = 0; p != reg->unk_28; p++) {
                            if ((op->unk_07 >> p) & 1) {
                                cls0805c244 *oldL = (cls0805c244 *)bestReg->func_0805a908(p);
                                cls0805c244 *newL = (cls0805c244 *)newReg->func_0805a908(p);
                                if (!oldL->removeOutput(&op->unk_2c[p])) {
                                    func_0804ad38(getMoveRetargetDeregisterError);
                                    return -1;
                                }
                                if (!newL->addOutput(&op->unk_2c[p], it->unk_13cf, op->unk_0a, op->unk_0b)) {
                                    func_0804ad38(getMoveRetargetReregisterError);
                                    return -1;
                                }
                                op->unk_18 = newReg;
                            }
                        }
                    }
                }
            }
        }
        int g1 = g_080b33ec;
        int zm0 = (unsigned char)in->unk_138b;
        zm0 >>= 4;
        zm0 >>= 4;
        if (zm0)
            goto no_move;
        int zm1 = (unsigned char)out->unk_07;
        zm1 >>= 4;
        zm1 >>= 4;
        int m = moved + zm1;
        int zm2 = (unsigned char)mvIn->unk_07;
        zm2 >>= 4;
        zm2 >>= 4;
        if (zm2)
            goto no_move;
        g_080b33ec = g1;
        if (!m) {
            addFreeMove(mv);
            g_080b33ec++;
            goto no_move;
        }
        mv->unk_138f = moved;
        mvIn->unk_07 = moved;
        out->unk_07 = moved;
        if (!AddToDo_Head(blk, mv)) {
            func_0804ad38(getMoveTodoHeadError);
            return -1;
        }
        logInstr(blk, mv);
        countCreatedMove(blk, mv);
        return 0;
    }
}
