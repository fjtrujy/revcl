#include "iInfo_shared.h"

#include "Instr.h"
#include "Block.h"
#include "os.h"


Instr *makeMove(Block *blk, RegPoolEntry *reg)
{
    int type = 0;
    Instr *mv = 0;
    if (blk->oddPipeMoveCount + blk->evenPipeMoveCount + blk->vectorMoveCount >= 2 * blk->unk_38)
        return 0;
    {
        switch (reg->registerClass) {
        case 8:
            type = 1;
            if (blk->evenPipeMoveCount >= blk->unk_38) {
                blk->unk_ad = 1;
                return mv;
            }
            break;
        case 9:
            type = 2;
            break;
        default:
            reportError("can't make move for reg %s\n", reg->registerName());
            abortAfterError();
        }
        mv = getMove(type, 0);
        if (mv == 0) {
            reportError("failed to get a free move\n");
            abortAfterError();
        }
        Operand *in = mv->getOp(1);
        if (!in) {
            reportError("failed to get move input!\n");
            exitAfterCleanup(1);
        }
        Operand *out = mv->getOp(0);
        if (!out) {
            reportError("failed to get move output!\n");
            exitAfterCleanup(1);
        }
        RegPoolEntry *dup = out->unk_18;
        if (!dup) {
            reportError("failed to dup  a reg\n");
            abortAfterError();
        }
        mv->unk_141a = blk->unk_38;
        in->unk_18 = reg;
        in->unk_2b = 1;
        in->unk_10 = 0;
        out->unk_2b = 1;
        out->unk_10 = 0;
        mv->unk_13d1 = 1;
        mv->unk_138e = 0;
    }
    return mv;
}

void adjustMoveCount(Block *blk, Instr *mv, int);
bool addFreeMove(Instr *mv);
bool setPipeBits(Instr *in);
bool AddToDo_Head(Block *blk, Instr *n);
bool logInstr(Block *blk, Instr *in);
void func_080737b8(Instr *in);

short g_streamSerial = 1;
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
    OperandLock *bestLk = 0;
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
        if (reg && isVuRegister(reg) && op2->unk_2b != 2) {
            OperandLock *lk = 0;
            int score = -1;
            for (int p = 0; p != reg->partCount; p++) {
                if ((op2->unk_07 >> p) & 1) {
                    lk = &op2->unk_2c[p];
                    OperandLock *parent = lk->getParent();
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
                                reportError("parent not inserted during move\n");
                                abortAfterError();
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
                            reportError("dff was <0\n");
                            abortAfterError();
                        }
                        RegPoolEntry *uA = op2->unk_10;
                        RegisterPartLockList *u04 = parent->unk_04;
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
            Instr *s2 = blk->pendingInstructions.head;
            if (score > bestScore) {
                in->unk_00 = s1;
                blk->pendingInstructions.head = s2;
                bestOp = op2;
                bestScore = score;
                bestReg = reg;
                bestLk = lk;
            }
        }
    }
    op = 0;
    if (!bestReg) {
        reportWarning("getMove: didn't set rg_old during pass\n");
        return 4;
    }
    OperandLock *parent = bestLk->getParent();
    if (!parent) {
        reportError("getMove: failed to get parent for something I wanted to move!\n");
        return -1;
    }
    Instr *owner = parent->ownerInstr();
    int mask = 0;
    int p;
    for (p = 0; p != bestReg->partCount; p++) {
        OperandLock *o = ((RegisterPartLockList *)bestReg->getPartPayload(p))->unk_1c;
        if (o && o->unk_22)
            mask |= 1 << p;
    }
    Instr *mv = makeMove(blk, bestReg);
    if (!mv) {
    no_move:
        Instr *earliest = 0;
        for (p = 0; p != bestReg->partCount; p++) {
            OperandLock *o = ((RegisterPartLockList *)bestReg->getPartPayload(p))->unk_1c;
            if (((mask >> p) & 1) && o && o->unk_22) {
                Instr *oi = o->ownerInstr();
                if (oi && (!earliest || oi->position.getRow() < earliest->position.getRow()))
                    earliest = oi;
            }
        }
        if (!earliest)
            earliest = blk->completedInstructions.head;
        func_080737b8(earliest);
        return 1;
    }
    {
        setPipeBits(mv);
        if (owner) {
            mv->linearOrderIndex = owner->linearOrderIndex + 1;
            mv->unk_1432 = owner->unk_1432 + 1;
            mv->unk_141a = owner->unk_141a;
            mv->unk_13f0.copyCountersFrom(&owner->unk_13f0);
        } else {
            mv->unk_1432 = 1;
            mv->linearOrderIndex = in->linearOrderIndex - 1;
            mv->unk_141a = in->unk_141a;
            mv->unk_13f0.copyCountersFrom(&in->unk_13f0);
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
        for (Instr *it = blk->pendingInstructions.head; it; it = (Instr *)it->unk_24.unk_00) {
            int n;
            for (n = 0; n != it->operandCount; n++) {
                Operand *op = &it->operands[n];
                RegPoolEntry *reg = registerOperand(op, 1);
                if (reg == bestReg) {
                    bool stale = false;
                    if (op->unk_2b != 1) {
                        if (op->unk_07 & newBits) {
                            reportError("cant age input - but the old register status is now invalid\n");
                            abortAfterError();
                        }
                        stale = true;
                    }
                    if (op->unk_07 & oldBits) {
                        if (op->unk_07 & newBits) {
                            reportError("neither old new bits are valid - valid register state is spread over too many different registers\n");
                            abortAfterError();
                        }
                        stale = true;
                    }
                    if (stale) {
                        mv->unk_13d1 = 0;
                        continue;
                    }
                    for (int p = 0; p != reg->partCount; p++) {
                        if ((op->unk_07 >> p) & 1) {
                            OperandLock *lk = &op->unk_2c[p];
                            OperandLock *par = lk->getParent();
                            RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                            RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                            if (par->unk_2c) {
                                OperandLock *oldOut = oldL->unk_1c;
                                if (!((moved >> p) & 1)) {
                                    OperandLock *moveInLock = &mvIn->unk_2c[p];
                                    if (!moveInLock->setOwner(p, mv, 1)) {
                                        reportError("getMove : failed to set owner for input\n");
                                        return -1;
                                    }
                                    if (!out->unk_2c[p].setOwner(p, mv, 0)) {
                                        reportError("getMove :failed to set owner for output\n");
                                        return -1;
                                    }
                                    OperandLock *moveOutLock = &out->unk_2c[p];
                                    if (!oldL->addInput(oldOut, moveInLock, mvIn->unk_29, mvIn->unk_0b)) {
                                        reportError("failed to register input from move instruction to old output\n");
                                        return -1;
                                    }
                                    moveInLock->unk_19 = mvIn->getStatus();
                                    if (!newL->addOutput(moveOutLock, mv->unk_13cf, out->unk_0a, out->unk_0b)) {
                                        reportError("getMove : failed to register output lock for move\n");
                                        return -1;
                                    }
                                    moveOutLock->unk_19 = out->getStatus();
                                }
                                moved |= 1 << p;
                                if (!oldL->removeInput(lk)) {
                                    reportError("failed to deregister an input during move create\n");
                                    return -1;
                                }
                                if (!newL->addInput(&out->unk_2c[p], lk, op->unk_29, op->unk_0b)) {
                                    reportError("failed to register input to a moves output\n");
                                    return -1;
                                }
                                op->unk_18 = newReg;
                            } else {
                                if (par->unk_04 == newL) {
                                    if (!oldL->removeInput(lk)) {
                                        reportError("failed to deregister an input during move create\n");
                                        return -1;
                                    }
                                    if (!newL->addInput(par, lk, op->unk_29, op->unk_0b)) {
                                        reportError("failed to register an input during move create\n");
                                        return -1;
                                    }
                                    op->unk_18 = newReg;
                                } else {
                                    reportError("hmm candidate for moving wasn't one after all\n");
                                    abortAfterError();
                                }
                            }
                        }
                    }
                }
            }
            for (n = 0; n != it->operandCount; n++) {
                Operand *op = &it->operands[n];
                RegPoolEntry *reg = registerOperand(op, 2);
                if (reg == bestReg) {
                    if (op->unk_2b != 1) {
                        mv->unk_13d1 = 0;
                        oldBits |= op->unk_07;
                    } else {
                        newBits |= op->unk_07;
                        for (int p = 0; p != reg->partCount; p++) {
                            if ((op->unk_07 >> p) & 1) {
                                RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                                RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                                if (!oldL->removeOutput(&op->unk_2c[p])) {
                                    reportError("retargetOutputOperands: failed to deregister output\n");
                                    return -1;
                                }
                                if (!newL->addOutput(&op->unk_2c[p], it->unk_13cf, op->unk_0a, op->unk_0b)) {
                                    reportError("retargetOutputOperands : failed to reregister output\n");
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
        int zm0 = (unsigned char)in->operandCount;
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
        mv->destinationMask = moved;
        mvIn->unk_07 = moved;
        out->unk_07 = moved;
        if (!AddToDo_Head(blk, mv)) {
            reportError("getMove : failed to add move to the head of the to todo list\n");
            return -1;
        }
        logInstr(blk, mv);
        countCreatedMove(blk, mv);
        return 0;
    }
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
        reportError("failed to unlink from todolist!\n");
    for (Instr *i = blk->pendingInstructions.head; i; i = (Instr *)i->unk_24.unk_00) {
        for (int n = 0; n != i->operandCount; n++) {
            Operand *op = &i->operands[n];
            RegPoolEntry *r = op->unk_18;
            if (r == dup) {
                op->unk_18 = orig;
                if (isRegisterOperand(op, 1)) {
                    for (int p = 0; p != r->partCount; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            OperandLock *lk = &op->unk_2c[p];
                            OperandLock *parent = lk->getParent();
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(p);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(p);
                            if ((bool)((out->unk_07 >> p) & 1) && parent == &out->unk_2c[p])
                                parent = in->unk_2c[p].getParent();
                            if (!oldList->removeInput(lk)) {
                                reportError("failed to deregister input lock for undo move\n");
                                return true;
                            }
                            if (!newList->addInput(parent, lk, op->unk_29, op->unk_0b)) {
                                reportError("undo move - failed to register input\n");
                                return true;
                            }
                        }
                    }
                }
                if (isRegisterOperand(op, 2)) {
                    for (int p = 0; p != r->partCount; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(p);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(p);
                            if (!oldList->removeOutput(&op->unk_2c[p])) {
                                reportError("failed to deregister output lock for undo move\n");
                                return true;
                            }
                            if (!newList->addOutput(&op->unk_2c[p], i->unk_13cf, op->unk_0a, op->unk_0b)) {
                                reportError("undo move - failed to register output\n");
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    for (int p = 0; p != dup->partCount; p++) {
        if ((bool)((in->unk_07 >> p) & 1)) {
            RegisterPartLockList *l = (RegisterPartLockList *)orig->getPartPayload(p);
            if (!l) {
                reportError("failed to get lock list for old reg in move undo\n");
                abortAfterError();
            }
            if (!in->unk_2c[p].getParent()) {
                reportError("undoMove: failed to get parent for old lock!\n");
                abortAfterError();
            }
            if (!l->removeInput(&in->unk_2c[p])) {
                reportError("undoMove : failed to register input lock\n");
                abortAfterError();
            }
        }
    }
    if (mv->operandCount != 2) {
        reportError("move with >2 operands\n");
        abortAfterError();
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

bool undoMovesTo(Block *blk, Instr *stop)
{
    bool done;
    Instr *i = blk->pendingInstructions.tail;
    do {
        Instr *prev = i->unk_24.unk_04;
        done = (i == stop);
        if (i->unk_1428) {
            if (!undoMove(blk, i)) {
                reportError("failed to undo move !\n");
                abortAfterError();
            }
        }
        i = prev;
    } while (!done);
    return true;
}

extern const char moveRecMismatchError[] = "getFreeMove - mr not same %x!=%x\n";
extern const char moveRecNullRllError[] = "getFreeLocalMove() - null rll pointer in cached move record\n";
extern const char moveRecSidefxError[] = "add code to handle sidefx for this move target\n";
extern const char moveRecNewInstrError[] = "failed to get a new instruction for move\n";
extern const char moveRecBaseMuidError[] = "baseMuid is not set for free move\n";
extern const char moveDetectNoRegError[] = "move detect error - reg not set\n";
