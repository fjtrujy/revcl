#include "iInfo_shared.h"

#include "Instr.h"
#include "Block.h"
#include "os.h"


Instr *makeMove(Block *blk, RegPoolEntry *reg)
{
    int type = 0;
    Instr *mv = 0;
    if (blk->oddPipeMoveCount + blk->evenPipeMoveCount + blk->vectorMoveCount >= 2 * blk->loopSize)
        return 0;
    {
        switch (reg->registerClass) {
        case 8:
            type = 1;
            if (blk->evenPipeMoveCount >= blk->loopSize) {
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
        RegPoolEntry *dup = out->assignedRegister;
        if (!dup) {
            reportError("failed to dup  a reg\n");
            abortAfterError();
        }
        mv->candidateAttemptLimit = blk->loopSize;
        in->assignedRegister = reg;
        in->operandStatus = 1;
        in->forcedBaseRegister = 0;
        out->operandStatus = 1;
        out->forcedBaseRegister = 0;
        mv->unk_13d1 = 1;
        mv->schedulerCategory = 0;
    }
    return mv;
}

void adjustMoveCount(Block *blk, Instr *mv, int);
bool addFreeMove(Instr *mv);
bool setPipeBits(Instr *in);
bool AddToDo_Head(Block *blk, Instr *n);
bool logInstr(Block *blk, Instr *in);
void trackEarliestInsertion(Instr *in);

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
    if (in->moveRecordWord)
        return 2;
    Operand *op;
    int i;
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            r->minimumMoveRow = 0x7fff;
            r->maximumMoveRow = -1;
        }
    }
    Operand *op2;
    for (i = 0; (op2 = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op2, 1);
        if (reg && isVuRegister(reg) && op2->operandStatus != 2) {
            OperandLock *lk = 0;
            int score = -1;
            for (int p = 0; p != reg->partCount; p++) {
                if ((op2->componentMask >> p) & 1) {
                    lk = &op2->parts[p];
                    OperandLock *parent = lk->getParent();
                    if (lk->getPriority() == 2 && score < 10000)
                        score = 10000;
                    if (lk->getPriority() == 1 && score < 20000)
                        score = 20000;
                    if (parent->ownerType != 2) {
                        int row = parent->unk_32;
                        int z = op2->componentMask;
                        if (row < 0) {
                            z >>= 4;
                            z >>= 4;
                            if (z <= 0) {
                                reportError("parent not inserted during move\n");
                                abortAfterError();
                            }
                        }
                        int lo = row + 1;
                        int hi = row + parent->issueLatency;
                        if (lo < reg->minimumMoveRow)
                            reg->minimumMoveRow = lo;
                        if (hi > reg->maximumMoveRow)
                            reg->maximumMoveRow = hi;
                        int d = now - lo;
                        if (d < 0) {
                            reportError("dff was <0\n");
                            abortAfterError();
                        }
                        RegPoolEntry *uA = op2->forcedBaseRegister;
                        RegisterPartLockList *u04 = parent->ownerLockList;
                        if (d > score) {
                            op2->forcedBaseRegister = uA;
                            parent->ownerLockList = u04;
                            score = d;
                        }
                        RegPoolEntry *u14b = op2->originalRegister;
                        op2->originalRegister = u14b;
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
        OperandLock *o = ((RegisterPartLockList *)bestReg->getPartPayload(p))->outputLocks;
        if (o && o->unk_22)
            mask |= 1 << p;
    }
    Instr *mv = makeMove(blk, bestReg);
    if (!mv) {
    no_move:
        Instr *earliest = 0;
        for (p = 0; p != bestReg->partCount; p++) {
            OperandLock *o = ((RegisterPartLockList *)bestReg->getPartPayload(p))->outputLocks;
            if (((mask >> p) & 1) && o && o->unk_22) {
                Instr *oi = o->ownerInstr();
                if (oi && (!earliest || oi->position.getRow() < earliest->position.getRow()))
                    earliest = oi;
            }
        }
        if (!earliest)
            earliest = blk->completedInstructions.head;
        trackEarliestInsertion(earliest);
        return 1;
    }
    {
        setPipeBits(mv);
        if (owner) {
            mv->linearOrderIndex = owner->linearOrderIndex + 1;
            mv->moveChainDepth = owner->moveChainDepth + 1;
            mv->candidateAttemptLimit = owner->candidateAttemptLimit;
            mv->schedulingStatistics.copyCountersFrom(&owner->schedulingStatistics);
        } else {
            mv->moveChainDepth = 1;
            mv->linearOrderIndex = in->linearOrderIndex - 1;
            mv->candidateAttemptLimit = in->candidateAttemptLimit;
            mv->schedulingStatistics.copyCountersFrom(&in->schedulingStatistics);
        }
        mv->candidateCostOffset = 0;
        Operand *out = mv->getOp(0);
        Operand *mvIn = mv->getOp(1);
        RegPoolEntry *newReg = out->assignedRegister;
        mvIn->originalRegister = bestOp->originalRegister;
        out->originalRegister = newReg;
        short targetRow = out->outputLatency;
        mv->moveRowOffset = now - targetRow;
        int moved = 0;
        int oldBits = 0;
        int newBits = 0;
        for (Instr *it = blk->pendingInstructions.head; it; it = (Instr *)it->pendingLink.nextInstruction) {
            int n;
            for (n = 0; n != it->operandCount; n++) {
                Operand *op = &it->operands[n];
                RegPoolEntry *reg = registerOperand(op, 1);
                if (reg == bestReg) {
                    bool stale = false;
                    if (op->operandStatus != 1) {
                        if (op->componentMask & newBits) {
                            reportError("cant age input - but the old register status is now invalid\n");
                            abortAfterError();
                        }
                        stale = true;
                    }
                    if (op->componentMask & oldBits) {
                        if (op->componentMask & newBits) {
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
                        if ((op->componentMask >> p) & 1) {
                            OperandLock *lk = &op->parts[p];
                            OperandLock *par = lk->getParent();
                            RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                            RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                            if (par->unk_2c) {
                                OperandLock *oldOut = oldL->outputLocks;
                                if (!((moved >> p) & 1)) {
                                    OperandLock *moveInLock = &mvIn->parts[p];
                                    if (!moveInLock->setOwner(p, mv, 1)) {
                                        reportError("getMove : failed to set owner for input\n");
                                        return -1;
                                    }
                                    if (!out->parts[p].setOwner(p, mv, 0)) {
                                        reportError("getMove :failed to set owner for output\n");
                                        return -1;
                                    }
                                    OperandLock *moveOutLock = &out->parts[p];
                                    if (!oldL->addInput(oldOut, moveInLock, mvIn->additionalLatency, mvIn->unk_0b)) {
                                        reportError("failed to register input from move instruction to old output\n");
                                        return -1;
                                    }
                                    moveInLock->unk_19 = mvIn->getStatus();
                                    if (!newL->addOutput(moveOutLock, mv->baseIssueSpacing, out->outputLatency, out->unk_0b)) {
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
                                if (!newL->addInput(&out->parts[p], lk, op->additionalLatency, op->unk_0b)) {
                                    reportError("failed to register input to a moves output\n");
                                    return -1;
                                }
                                op->assignedRegister = newReg;
                            } else {
                                if (par->ownerLockList == newL) {
                                    if (!oldL->removeInput(lk)) {
                                        reportError("failed to deregister an input during move create\n");
                                        return -1;
                                    }
                                    if (!newL->addInput(par, lk, op->additionalLatency, op->unk_0b)) {
                                        reportError("failed to register an input during move create\n");
                                        return -1;
                                    }
                                    op->assignedRegister = newReg;
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
                    if (op->operandStatus != 1) {
                        mv->unk_13d1 = 0;
                        oldBits |= op->componentMask;
                    } else {
                        newBits |= op->componentMask;
                        for (int p = 0; p != reg->partCount; p++) {
                            if ((op->componentMask >> p) & 1) {
                                RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                                RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                                if (!oldL->removeOutput(&op->parts[p])) {
                                    reportError("retargetOutputOperands: failed to deregister output\n");
                                    return -1;
                                }
                                if (!newL->addOutput(&op->parts[p], it->baseIssueSpacing, op->outputLatency, op->unk_0b)) {
                                    reportError("retargetOutputOperands : failed to reregister output\n");
                                    return -1;
                                }
                                op->assignedRegister = newReg;
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
        int zm1 = (unsigned char)out->componentMask;
        zm1 >>= 4;
        zm1 >>= 4;
        int m = moved + zm1;
        int zm2 = (unsigned char)mvIn->componentMask;
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
        mvIn->componentMask = moved;
        out->componentMask = moved;
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
    if (mv->concreteMoveOpcode)
        unresolveMove(blk, mv);
    Operand *out = mv->getOp(0);
    Operand *in = mv->getOp(1);
    RegPoolEntry *orig = in->assignedRegister;
    RegPoolEntry *dup = out->assignedRegister;
    if (!UnlinkFromToDo(blk, mv))
        reportError("failed to unlink from todolist!\n");
    for (Instr *i = blk->pendingInstructions.head; i; i = (Instr *)i->pendingLink.nextInstruction) {
        for (int n = 0; n != i->operandCount; n++) {
            Operand *op = &i->operands[n];
            RegPoolEntry *r = op->assignedRegister;
            if (r == dup) {
                op->assignedRegister = orig;
                if (isRegisterOperand(op, 1)) {
                    for (int p = 0; p != r->partCount; p++) {
                        if ((bool)((op->componentMask >> p) & 1)) {
                            OperandLock *lk = &op->parts[p];
                            OperandLock *parent = lk->getParent();
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(p);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(p);
                            if ((bool)((out->componentMask >> p) & 1) && parent == &out->parts[p])
                                parent = in->parts[p].getParent();
                            if (!oldList->removeInput(lk)) {
                                reportError("failed to deregister input lock for undo move\n");
                                return true;
                            }
                            if (!newList->addInput(parent, lk, op->additionalLatency, op->unk_0b)) {
                                reportError("undo move - failed to register input\n");
                                return true;
                            }
                        }
                    }
                }
                if (isRegisterOperand(op, 2)) {
                    for (int p = 0; p != r->partCount; p++) {
                        if ((bool)((op->componentMask >> p) & 1)) {
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(p);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(p);
                            if (!oldList->removeOutput(&op->parts[p])) {
                                reportError("failed to deregister output lock for undo move\n");
                                return true;
                            }
                            if (!newList->addOutput(&op->parts[p], i->baseIssueSpacing, op->outputLatency, op->unk_0b)) {
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
        if ((bool)((in->componentMask >> p) & 1)) {
            RegisterPartLockList *l = (RegisterPartLockList *)orig->getPartPayload(p);
            if (!l) {
                reportError("failed to get lock list for old reg in move undo\n");
                abortAfterError();
            }
            if (!in->parts[p].getParent()) {
                reportError("undoMove: failed to get parent for old lock!\n");
                abortAfterError();
            }
            if (!l->removeInput(&in->parts[p])) {
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
        Instr *prev = i->pendingLink.previousInstruction;
        done = (i == stop);
        if (i->moveRecordWord) {
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
