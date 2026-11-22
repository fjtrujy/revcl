#include "iInfo_shared.h"

#include "Instr.h"
#include "Block.h"
#include "os.h"


Instr *makeMove(Block *blk, RegPoolEntry *reg)
{
    int type = 0;
    Instr *instruction = 0;
    if (blk->oddPipeMoveCount + blk->evenPipeMoveCount + blk->vectorMoveCount >= 2 * blk->loopSize)
        return 0;
    {
        switch (reg->registerClass) {
        case 8:
            type = 1;
            if (blk->evenPipeMoveCount >= blk->loopSize) {
                blk->unk_ad = 1;
                return instruction;
            }
            break;
        case 9:
            type = 2;
            break;
        default:
            reportError("can't make move for reg %s\n", reg->registerName());
            abortAfterError();
        }
        instruction = getMove(type, 0);
        if (instruction == 0) {
            reportError("failed to get a free move\n");
            abortAfterError();
        }
        Operand *inputOperand = instruction->getOp(1);
        if (!inputOperand) {
            reportError("failed to get move input!\n");
            exitAfterCleanup(1);
        }
        Operand *out = instruction->getOp(0);
        if (!out) {
            reportError("failed to get move output!\n");
            exitAfterCleanup(1);
        }
        RegPoolEntry *dup = out->assignedRegister;
        if (!dup) {
            reportError("failed to dup  a reg\n");
            abortAfterError();
        }
        instruction->candidateAttemptLimit = blk->loopSize;
        inputOperand->assignedRegister = reg;
        inputOperand->operandStatus = 1;
        inputOperand->forcedBaseRegister = 0;
        out->operandStatus = 1;
        out->forcedBaseRegister = 0;
        instruction->requiresFullLoopMoveDrag = 1;
        instruction->schedulerCategory = 0;
    }
    return instruction;
}

void adjustMoveCount(Block *blk, Instr *mv, int);
bool addFreeMove(Instr *mv);
bool setPipeBits(Instr *in);
bool AddToDo_Head(Block *blk, Instr *n);
bool logInstr(Block *blk, Instr *in);
void trackEarliestInsertion(Instr *in);

short g_streamSerial = 1;
int discardedEmptyMoveCount = 0;

static inline void countCreatedMove(Block *blk, Instr *instruction)
{
    adjustMoveCount(blk, instruction, 1);
}

int getMove(Block *blk, Instr *instruction, int now)
{
    Operand *bestOp = 0;
    int bestScore = -1;
    RegPoolEntry *bestReg = 0;
    OperandLock *bestLk = 0;
    if (instruction->moveRecordWord)
        return 2;
    Operand *operand;
    int i;
    for (i = 0; (operand = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *registerEntry = registerOperand(operand, 1);
        if (registerEntry) {
            registerEntry->minimumMoveRow = 0x7fff;
            registerEntry->maximumMoveRow = -1;
        }
    }
    Operand *op2;
    for (i = 0; (op2 = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op2, 1);
        if (reg && isVuRegister(reg) && op2->operandStatus != 2) {
            OperandLock *lock = 0;
            int score = -1;
            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                if ((op2->componentMask >> partIndex) & 1) {
                    lock = &op2->parts[partIndex];
                    OperandLock *parent = lock->getParent();
                    if (lock->getPriority() == 2 && score < 10000)
                        score = 10000;
                    if (lock->getPriority() == 1 && score < 20000)
                        score = 20000;
                    if (parent->ownerType != 2) {
                        int row = parent->linearRow;
                        int z = op2->componentMask;
                        if (row < 0) {
                            z >>= 4;
                            z >>= 4;
                            if (z <= 0) {
                                reportError("parent not inserted during move\n");
                                abortAfterError();
                            }
                        }
                        int earliestMoveRow = row + 1;
                        int latestMoveRow = row + parent->issueLatency;
                        if (earliestMoveRow < reg->minimumMoveRow)
                            reg->minimumMoveRow = earliestMoveRow;
                        if (latestMoveRow > reg->maximumMoveRow)
                            reg->maximumMoveRow = latestMoveRow;
                        int distanceToUse = now - earliestMoveRow;
                        if (distanceToUse < 0) {
                            reportError("dff was <0\n");
                            abortAfterError();
                        }
                        RegPoolEntry *originalBaseRegister = op2->forcedBaseRegister;
                        RegisterPartLockList *u04 = parent->ownerLockList;
                        if (distanceToUse > score) {
                            op2->forcedBaseRegister = originalBaseRegister;
                            parent->ownerLockList = u04;
                            score = distanceToUse;
                        }
                        RegPoolEntry *u14b = op2->originalRegister;
                        op2->originalRegister = u14b;
                    }
                }
            }
            int s1 = instruction->optimizedInstructionImageWord;
            Instr *s2 = blk->pendingInstructions.head;
            if (score > bestScore) {
                instruction->optimizedInstructionImageWord = s1;
                blk->pendingInstructions.head = s2;
                bestOp = op2;
                bestScore = score;
                bestReg = reg;
                bestLk = lock;
            }
        }
    }
    operand = 0;
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
        OperandLock *outputLock = ((RegisterPartLockList *)bestReg->getPartPayload(p))->outputLocks;
        if (outputLock && outputLock->activeReferenceCount)
            mask |= 1 << p;
    }
    Instr *moveInstruction = makeMove(blk, bestReg);
    if (!moveInstruction) {
    no_move:
        Instr *earliest = 0;
        for (p = 0; p != bestReg->partCount; p++) {
            OperandLock *outputLock = ((RegisterPartLockList *)bestReg->getPartPayload(p))->outputLocks;
            if (((mask >> p) & 1) && outputLock && outputLock->activeReferenceCount) {
                Instr *otherInstruction = outputLock->ownerInstr();
                if (otherInstruction && (!earliest || otherInstruction->position.getRow() < earliest->position.getRow()))
                    earliest = otherInstruction;
            }
        }
        if (!earliest)
            earliest = blk->completedInstructions.head;
        trackEarliestInsertion(earliest);
        return 1;
    }
    {
        setPipeBits(moveInstruction);
        if (owner) {
            moveInstruction->linearOrderIndex = owner->linearOrderIndex + 1;
            moveInstruction->moveChainDepth = owner->moveChainDepth + 1;
            moveInstruction->candidateAttemptLimit = owner->candidateAttemptLimit;
            moveInstruction->schedulingStatistics.copyCountersFrom(&owner->schedulingStatistics);
        } else {
            moveInstruction->moveChainDepth = 1;
            moveInstruction->linearOrderIndex = instruction->linearOrderIndex - 1;
            moveInstruction->candidateAttemptLimit = instruction->candidateAttemptLimit;
            moveInstruction->schedulingStatistics.copyCountersFrom(&instruction->schedulingStatistics);
        }
        moveInstruction->candidateCostOffset = 0;
        Operand *out = moveInstruction->getOp(0);
        Operand *mvIn = moveInstruction->getOp(1);
        RegPoolEntry *newReg = out->assignedRegister;
        mvIn->originalRegister = bestOp->originalRegister;
        out->originalRegister = newReg;
        short targetRow = out->outputLatency;
        moveInstruction->moveRowOffset = now - targetRow;
        int moved = 0;
        int oldBits = 0;
        int newBits = 0;
        for (Instr *pendingInstruction = blk->pendingInstructions.head; pendingInstruction; pendingInstruction = (Instr *)pendingInstruction->pendingLink.nextInstruction) {
            int operandIndex;
            for (operandIndex = 0; operandIndex != pendingInstruction->operandCount; operandIndex++) {
                Operand *operand = &pendingInstruction->operands[operandIndex];
                RegPoolEntry *reg = registerOperand(operand, 1);
                if (reg == bestReg) {
                    bool stale = false;
                    if (operand->operandStatus != 1) {
                        if (operand->componentMask & newBits) {
                            reportError("cant age input - but the old register status is now invalid\n");
                            abortAfterError();
                        }
                        stale = true;
                    }
                    if (operand->componentMask & oldBits) {
                        if (operand->componentMask & newBits) {
                            reportError("neither old new bits are valid - valid register state is spread over too many different registers\n");
                            abortAfterError();
                        }
                        stale = true;
                    }
                    if (stale) {
                        moveInstruction->requiresFullLoopMoveDrag = 0;
                        continue;
                    }
                    for (int p = 0; p != reg->partCount; p++) {
                        if ((operand->componentMask >> p) & 1) {
                            OperandLock *lock = &operand->parts[p];
                            OperandLock *par = lock->getParent();
                            RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                            RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                            if (par->insertedIntoLockList) {
                                OperandLock *oldOut = oldL->outputLocks;
                                if (!((moved >> p) & 1)) {
                                    OperandLock *moveInLock = &mvIn->parts[p];
                                    if (!moveInLock->setOwner(p, moveInstruction, 1)) {
                                        reportError("getMove : failed to set owner for input\n");
                                        return -1;
                                    }
                                    if (!out->parts[p].setOwner(p, moveInstruction, 0)) {
                                        reportError("getMove :failed to set owner for output\n");
                                        return -1;
                                    }
                                    OperandLock *moveOutLock = &out->parts[p];
                                    if (!oldL->addInput(oldOut, moveInLock, mvIn->additionalLatency, mvIn->externalBlockModeFlag)) {
                                        reportError("failed to register input from move instruction to old output\n");
                                        return -1;
                                    }
                                    moveInLock->operandStatusCode = mvIn->getStatus();
                                    if (!newL->addOutput(moveOutLock, moveInstruction->baseIssueSpacing, out->outputLatency, out->externalBlockModeFlag)) {
                                        reportError("getMove : failed to register output lock for move\n");
                                        return -1;
                                    }
                                    moveOutLock->operandStatusCode = out->getStatus();
                                }
                                moved |= 1 << p;
                                if (!oldL->removeInput(lock)) {
                                    reportError("failed to deregister an input during move create\n");
                                    return -1;
                                }
                                if (!newL->addInput(&out->parts[p], lock, operand->additionalLatency, operand->externalBlockModeFlag)) {
                                    reportError("failed to register input to a moves output\n");
                                    return -1;
                                }
                                operand->assignedRegister = newReg;
                            } else {
                                if (par->ownerLockList == newL) {
                                    if (!oldL->removeInput(lock)) {
                                        reportError("failed to deregister an input during move create\n");
                                        return -1;
                                    }
                                    if (!newL->addInput(par, lock, operand->additionalLatency, operand->externalBlockModeFlag)) {
                                        reportError("failed to register an input during move create\n");
                                        return -1;
                                    }
                                    operand->assignedRegister = newReg;
                                } else {
                                    reportError("hmm candidate for moving wasn't one after all\n");
                                    abortAfterError();
                                }
                            }
                        }
                    }
                }
            }
            for (operandIndex = 0; operandIndex != pendingInstruction->operandCount; operandIndex++) {
                Operand *operand = &pendingInstruction->operands[operandIndex];
                RegPoolEntry *reg = registerOperand(operand, 2);
                if (reg == bestReg) {
                    if (operand->operandStatus != 1) {
                        moveInstruction->requiresFullLoopMoveDrag = 0;
                        oldBits |= operand->componentMask;
                    } else {
                        newBits |= operand->componentMask;
                        for (int p = 0; p != reg->partCount; p++) {
                            if ((operand->componentMask >> p) & 1) {
                                RegisterPartLockList *oldL = (RegisterPartLockList *)bestReg->getPartPayload(p);
                                RegisterPartLockList *newL = (RegisterPartLockList *)newReg->getPartPayload(p);
                                if (!oldL->removeOutput(&operand->parts[p])) {
                                    reportError("retargetOutputOperands: failed to deregister output\n");
                                    return -1;
                                }
                                if (!newL->addOutput(&operand->parts[p], pendingInstruction->baseIssueSpacing, operand->outputLatency, operand->externalBlockModeFlag)) {
                                    reportError("retargetOutputOperands : failed to reregister output\n");
                                    return -1;
                                }
                                operand->assignedRegister = newReg;
                            }
                        }
                    }
                }
            }
        }
        int g1 = discardedEmptyMoveCount;
        int zm0 = (unsigned char)instruction->operandCount;
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
        discardedEmptyMoveCount = g1;
        if (!m) {
            addFreeMove(moveInstruction);
            discardedEmptyMoveCount++;
            goto no_move;
        }
        moveInstruction->destinationMask = moved;
        mvIn->componentMask = moved;
        out->componentMask = moved;
        if (!AddToDo_Head(blk, moveInstruction)) {
            reportError("getMove : failed to add move to the head of the to todo list\n");
            return -1;
        }
        logInstr(blk, moveInstruction);
        countCreatedMove(blk, moveInstruction);
        return 0;
    }
}

int unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
void decrementInstructionPipelineCounts(Block *blk, Instr *mv);

bool undoMove(Block *blk, Instr *moveInstruction)
{
    if (moveInstruction->concreteMoveOpcode)
        unresolveMove(blk, moveInstruction);
    Operand *out = moveInstruction->getOp(0);
    Operand *inputOperand = moveInstruction->getOp(1);
    RegPoolEntry *orig = inputOperand->assignedRegister;
    RegPoolEntry *dup = out->assignedRegister;
    if (!UnlinkFromToDo(blk, moveInstruction))
        reportError("failed to unlink from todolist!\n");
    for (Instr *instruction = blk->pendingInstructions.head; instruction; instruction = (Instr *)instruction->pendingLink.nextInstruction) {
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegPoolEntry *registerEntry = operand->assignedRegister;
            if (registerEntry == dup) {
                operand->assignedRegister = orig;
                if (isRegisterOperand(operand, 1)) {
                    for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                        if ((bool)((operand->componentMask >> partIndex) & 1)) {
                            OperandLock *lock = &operand->parts[partIndex];
                            OperandLock *parent = lock->getParent();
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(partIndex);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(partIndex);
                            if ((bool)((out->componentMask >> partIndex) & 1) && parent == &out->parts[partIndex])
                                parent = inputOperand->parts[partIndex].getParent();
                            if (!oldList->removeInput(lock)) {
                                reportError("failed to deregister input lock for undo move\n");
                                return true;
                            }
                            if (!newList->addInput(parent, lock, operand->additionalLatency, operand->externalBlockModeFlag)) {
                                reportError("undo move - failed to register input\n");
                                return true;
                            }
                        }
                    }
                }
                if (isRegisterOperand(operand, 2)) {
                    for (int p = 0; p != registerEntry->partCount; p++) {
                        if ((bool)((operand->componentMask >> p) & 1)) {
                            RegisterPartLockList *newList = (RegisterPartLockList *)orig->getPartPayload(p);
                            RegisterPartLockList *oldList = (RegisterPartLockList *)dup->getPartPayload(p);
                            if (!oldList->removeOutput(&operand->parts[p])) {
                                reportError("failed to deregister output lock for undo move\n");
                                return true;
                            }
                            if (!newList->addOutput(&operand->parts[p], instruction->baseIssueSpacing, operand->outputLatency, operand->externalBlockModeFlag)) {
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
        if ((bool)((inputOperand->componentMask >> p) & 1)) {
            RegisterPartLockList *registerPartLocks = (RegisterPartLockList *)orig->getPartPayload(p);
            if (!registerPartLocks) {
                reportError("failed to get lock list for old reg in move undo\n");
                abortAfterError();
            }
            if (!inputOperand->parts[p].getParent()) {
                reportError("undoMove: failed to get parent for old lock!\n");
                abortAfterError();
            }
            if (!registerPartLocks->removeInput(&inputOperand->parts[p])) {
                reportError("undoMove : failed to register input lock\n");
                abortAfterError();
            }
        }
    }
    if (moveInstruction->operandCount != 2) {
        reportError("move with >2 operands\n");
        abortAfterError();
    }
    decrementInstructionPipelineCounts(blk, moveInstruction);
    adjustMoveCount(blk, moveInstruction, -1);
    addFreeMove(moveInstruction);
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
    Instr *instruction = blk->pendingInstructions.tail;
    do {
        Instr *prev = instruction->pendingLink.previousInstruction;
        done = (instruction == stop);
        if (instruction->moveRecordWord) {
            if (!undoMove(blk, instruction)) {
                reportError("failed to undo move !\n");
                abortAfterError();
            }
        }
        instruction = prev;
    } while (!done);
    return true;
}

extern const char moveRecMismatchError[] = "getFreeMove - mr not same %x!=%x\n";
extern const char moveRecNullRllError[] = "getFreeLocalMove() - null rll pointer in cached move record\n";
extern const char moveRecSidefxError[] = "add code to handle sidefx for this move target\n";
extern const char moveRecNewInstrError[] = "failed to get a new instruction for move\n";
extern const char moveRecBaseMuidError[] = "baseMuid is not set for free move\n";
extern const char moveDetectNoRegError[] = "move detect error - reg not set\n";
