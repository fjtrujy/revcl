#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

class Block;
bool calculateEarliestDependencyRow(Instr *i, int *row);
bool calculateLatestDependencyRow(Instr *i, int *row);
bool unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);
bool UnlinkFromDone(Block *blk, Instr *n);
bool AddToDo_Head(Block *blk, Instr *n);
bool setRange(int start, int end, int lsz);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int pullSpecialRegisterWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int pullLatencyWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos);
int countSetBits(unsigned int x);
extern bool staggerMemoryAccess;

static inline void dumpLockInstruction(Instr *in)
{
    in->dumpOperandDiagnostics(false);
}

bool setLocks(Instr *in)
{
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = op->assignedRegister;
        if (!r)
            continue;
        if (isRegisterOperand(op, 2)) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    RegisterPartLockList *ll = s->ownerLockList;
                    s->setFrom(&in->position);
                    if (!ll->addOutputLock(s)) {
                        reportError("oh no - add output lock failed %d.%d\n", i, p);
                        dumpLockInstruction(in);
                        return false;
                    }
                }
            }
        }
        if (isRegisterOperand(op, 1)) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    RegisterPartLockList *ll = s->ownerLockList;
                    s->setFrom(&in->position);
                    if (!ll->addInputLock(s)) {
                        reportError("SetLocks: oh no - add input lock failed %d.%d\n", i, p);
                        dumpLockInstruction(in);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool undoLocks(Instr *in)
{
    for (int i = in->operandCount - 1; i >= 0; i--) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = op->assignedRegister;
        if (!r)
            continue;
        if (isRegisterOperand(op, 2)) {
            for (int p = 0; p != r->partCount; p++) {
                op->parts[p].outputConflictMarked[0] = 0;
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    if (!s->ownerLockList->undoOutputLock(s))
                        return false;
                }
            }
        }
        if (isRegisterOperand(op, 1)) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    if (!s->ownerLockList->undoInputLock(s))
                        return false;
                }
            }
        }
    }
    return true;
}

int checkLocks(Instr *in, short *pos, int *minPos)
{
    OperandLock *saved[64];
    int ret = 0;
    int i;
    int n = 0;

    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = isRegisterOperand(op, 1) ? op->assignedRegister : 0;
        if (!r)
            continue;
        for (int p = 0; p != r->partCount; p++) {
            if ((op->componentMask >> p) & 1) {
                OpSlot *s = &op->parts[p];
                RegisterPartLockList *ll = s->ownerLockList;
                if (*minPos < ll->loopWrapMinimumRow) {
                    ret |= 0x80000;
                    *minPos = ll->loopWrapMinimumRow;
                    goto done;
                }
                int rc = ll->checkInputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x430) {
                    OperandLock *par = s->getParent();
                    Operand *o = par->ownerOperand();
                    if (o) {
                        Instr *oi = par->ownerInstr();
                        if (!o->parts[p].outputConflictMarked[0]) {
                            o->parts[p].outputConflictMarked[0] = 1;
                            o->parts[p].conflictingOutputLockCount++;
                        }
                        trackEarliestInsertion(oi);
                        oi->dumpOperandDiagnostics(false);
                    } else
                        reportWarning("failed to get setting operand .. so it's a regref??\n");
                    *minPos = 30000;
                    goto done;
                }
                if (s->pairedLock) {
                    OperandLock *par = s->getParent();
                    if (!par)
                        exitAfterCleanup(0);
                    par->decRef();
                    saved[n++] = par;
                }
            }
        }
    }
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = isRegisterOperand(op, 2) ? op->assignedRegister : 0;
        if (!r)
            continue;
        for (int p = 0; p != r->partCount; p++) {
            if ((op->componentMask >> p) & 1) {
                OpSlot *s = &op->parts[p];
                RegisterPartLockList *ll = op->parts[p].ownerLockList;
                if (op->parts[p].conflictingOutputLockCount) {
                    int v = ll->maxConstraintOfFirstOutputs(op->parts[p].conflictingOutputLockCount);
                    if (v > *minPos) {
                        ret |= 0x80000;
                        *minPos = v;
                        goto done;
                    }
                }
                int rc = ll->checkOutputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x22000) {
                    if (!activeLoopSize) {
                        reportError("late extern, loop overrun, but no loop\n");
                        abortAfterError();
                    }
                    int d = *pos + s->issueLatency - activeLoopSize;
                    if (d > 0) {
                        ll->loopWrapMinimumRow = d;
                        Instr *e = ll->earliestExt();
                        if (!e) {
                            reportWarning("failed to get earliest in ret=%x\n", rc);
                            ll->checkCounts();
                            reportError("this instruction is\n");
                            in->leDump(i, p);
                            abortAfterError();
                        } else
                            trackEarliestInsertion(e);
                        *minPos = 30000;
                        goto done;
                    }
                }
            }
        }
    }
done:
    for (int k = 0; k != n; k++)
        saved[k]->incRef();
    return ret;
}

int canMoveLocks(Instr *in, short *pos)
{
    OperandLock *saved[64];
    int ret = 0;
    int i;
    int n = 0;

    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (!r)
            continue;
        for (int p = 0; p != r->partCount; p++) {
            if ((op->componentMask >> p) & 1) {
                OperandLock *par = op->parts[p].getParent();
                if (!par->calcStats(in)) {
                    reportError("failed to recalc stats ignoring instruction\n");
                    abortAfterError();
                }
                saved[n++] = par;
            }
        }
    }
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    ret |= s->ownerLockList->canMoveInputLock(s, pos, (op->componentMask >> p) & 1);
                    if (ret)
                        goto done;
                }
            }
        }
        r = registerOperand(op, 2);
        if (r) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    ret |= s->ownerLockList->canMoveOutputLock(s, pos, (op->componentMask >> p) & 1);
                    if (ret)
                        goto done;
                }
            }
        }
    }
done:
    while (n != 0) {
        if (!saved[--n]->calcStats(0)) {
            reportError("failed to calc stats undoing move test\n");
            abortAfterError();
        }
    }
    return ret;
}

bool moveLocks(Instr *const in, InstructionPosition *pos)
{
    InstructionPosition *d = &in->position;
    d->linearRow = pos->linearRow;
    d->loopRow = pos->loopRow;
    d->loopIteration = pos->loopIteration;
    d->pipelineIndex = pos->pipelineIndex;
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = op->assignedRegister;
        if (!r)
            continue;
        if (isRegisterOperand(op, 2)) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    RegisterPartLockList *ll = s->ownerLockList;
                    s->setFrom(&in->position);
                    if (!ll->moveOutputLock(s)) {
                        reportError("oh no - move output lock failed %d.%d\n", i, p);
                        dumpLockInstruction(in);
                        abortAfterError();
                        return false;
                    }
                }
            }
        }
        if (isRegisterOperand(op, 1)) {
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *s = &op->parts[p];
                    RegisterPartLockList *ll = s->ownerLockList;
                    s->setFrom(&in->position);
                    if (!ll->moveInputLock(s)) {
                        reportError("oh no - move input lock failed %d.%d\n", i, p);
                        dumpLockInstruction(in);
                        abortAfterError();
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool setPipeBits(Instr *in)
{
    int i;
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        if (op->assignedRegister)
            op->assignedRegister->seenInputComponentMask = 0;
    }
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = op->inputRegister();
        if (!r)
            continue;
        for (int p = 0; p != r->partCount; p++) {
            if ((op->componentMask >> p) & 1) {
                OpSlot *s = &op->parts[p];
                s->upperPipeInputDemand = 0;
                s->lowerPipeInputDemand = 0;
                s->eitherPipeInputDemand = 0;
                if (!(((unsigned char)r->seenInputComponentMask >> p) & 1)) {
                    switch ((unsigned char)in->permittedPipelineMask) {
                    case 0:
                        reportError("no physical pipe bits!\n");
                        return false;
                    case 1:
                        s->upperPipeInputDemand = 1;
                        break;
                    case 2:
                        s->lowerPipeInputDemand = 1;
                        break;
                    case 3:
                        s->eitherPipeInputDemand = 1;
                        break;
                    default:
                        reportError("unknown pipe request\n");
                        return false;
                    }
                }
            }
        }
        r->seenInputComponentMask |= op->componentMask;
    }
    return true;
}

int checkInsert(workTable *wt, Instr *in, InstructionPosition *pos, int *minPos)
{
    int row = pos->linearRow;
    int pipe = pos->pipelineIndex;
    int r4 = pos->loopRow;

    if (*minPos < row)
        *minPos = row;

    short e = in->externalInputConstraint;
    if (e < in->externalInputPairedConstraint)
        e = in->externalInputPairedConstraint;
    int v = e;
    if (r4 < v + activeSchedulingRangeStart) {
        int m = *minPos;
        int next;
        if (!activeLoopSize)
            next = v;
        else {
            int rem = v % activeLoopSize;
            int q = m / activeLoopSize;
            if (rem <= m % activeLoopSize)
                q++;
            next = q * activeLoopSize + rem;
        }
        *minPos = next;
        return 0x10000;
    }

    short l = in->externalOutputConstraint;
    if (l < in->externalOutputPairedConstraint)
        l = in->externalOutputPairedConstraint;
    if (l && activeSchedulingRangeEnd >= 0 && activeSchedulingRangeEnd - r4 <= l) {
        *minPos = 30000;
        return 0x10000;
    }
    if (staggerMemoryAccess && in->ramAccessType && (bool)(r4 & 1)) {
        *minPos = row + 1;
        return 0x10000;
    }

    if (wt->loopSize) {
        if (row / wt->loopSize > 16) {
            *minPos = 30000;
            return 0x8000;
        }
    } else if (row >= wt->rowCapacity) {
        *minPos = 30000;
        return 0x8000;
    }

    int fr = 0;
    if (!in->hasExtraPredecessors) {
        if (!calculateEarliestDependencyRow(in, &fr)) {
            reportError("check fdeps failed\n");
            exitAfterCleanup(-1);
            return 0x80000000;
        }
        if (fr < 0) {
            *minPos = 30000;
            return 0x100000;
        }
        if (fr > *minPos) {
            *minPos = fr;
            return 0x100000;
        }
    }

    Instr **ie = ((cls_unk30 *)wt)->getIE_ck(pos->loopRow);
    if (ie[pipe] != (Instr *)in->replacedInstructionWord) {
        *minPos = row + 1;
        return 0x40;
    }
    if (!wouldPair(ie, in, pipe)) {
        (*minPos)++;
        return 0x80;
    }
    return checkLocks(in, &pos->linearRow, minPos);
}

bool DoInsert(workTable *const wt, Instr *in, InstructionPosition *pos)
{
    short b = in->abstractMoveOpcode;
    if (__builtin_expect(b != 0, 1) && !(b = in->concreteMoveOpcode)) {
        reportError("undo insert currmuid not set\n");
        return false;
    }
    short r = 0;
    char p = 0;
    {
        int zp = (unsigned char)pos->loopIteration;
        zp >>= 4;
        zp >>= 4;
        if (zp)
            return r != p;
    }
    r = pos->loopRow;
    wtRow *row = &wt->rows[r];
    InstructionPosition *s = &in->position;
    b = pos->linearRow;
    s->loopRow = r;
    char a = pos->loopIteration;
    s->loopIteration = a;
    p = pos->getPipe();
    s->linearRow = b;
    s->pipelineIndex = p;
    row->entries[p].instruction = in;
    b = setLocks(in);
    if (!(char)b) {
        reportError("DoInsert: failed to set locks in instruction insert\n");
        return false;
    }
    int zt;
    do {
        in->insertionSerial = wt->nextInsertionSerial;
        wt->nextInsertionSerial++;
        b = UnlinkFromToDo((Block *)wt, in);
        if (!(char)b) {
            reportError("failed to unlink from to do list\n");
            abortAfterError();
        }
        a = AddDone_Tail((Block *)wt, in);
        if (!a) {
            reportError("failed to add to done\n");
            abortAfterError();
        }
        zt = ((unsigned char)pos->pipelineIndex | (unsigned char)pos->loopIteration);
        zt >>= 4;
        zt >>= 4;
    } while (zt);
    return true;
}

bool UndoInsert(workTable *wt, Instr *in)
{
    if (in->branchFlags)
        wt->loopBoundaryInstruction = 0;
    if (in->abstractMoveOpcode) {
        if (!in->concreteMoveOpcode) {
            reportError("undo insert currmuid not set\n");
            return false;
        }
        if (!unresolveMove((Block *)wt, in)) {
            reportError("failed to unresolve move\n");
            abortAfterError();
        }
    }
    if (!wt->takeOut(in)) {
        reportError("failed to take out instruction!\n");
        abortAfterError();
    }
    if (!undoLocks(in)) {
        reportError("UndoInsert:: undo locks - failed\n");
        abortAfterError();
        return false;
    }
    wt->nextInsertionSerial--;
    in->insertionSerial = -1;
    if (!UnlinkFromDone((Block *)wt, in)) {
        reportError("failed to unlink from done list\n");
        abortAfterError();
    }
    if (!AddToDo_Head((Block *)wt, in)) {
        reportError("failed to relink to todo list\n");
        abortAfterError();
    }
    return true;
}

bool reduceExtents(workTable *wt)
{
    setRange(wt->rangeStart, wt->rangeEnd, wt->loopSize);
    int moved;
    do {
        moved = 0;
        for (int r = wt->rangeStart; __builtin_expect(r != wt->rangeEnd, 1); r++) {
            wtRow *row = wt->getRowByIndex(r);
            for (int p = 0; p != 2; p++) {
                Instr *in = row->getEntry(p);
                if (!in)
                    continue;
                int lo = in->position.linearRow;
                int cnt = 0;
                int minEnd = 30000;
                Operand *op;
                for (int i = 0; (op = in->getOp(i)) != 0; i++) {
                    RegPoolEntry *reg = registerOperand(op, 2);
                    if (reg) {
                        for (int k = 0; k != reg->partCount; k++) {
                            if ((op->componentMask >> k) & 1) {
                                cnt++;
                                OpSlot *s = &op->parts[k];
                                if (s->getRefCount() != 0) {
                                    int e = s->earliestConsumerFreeCycle;
                                    if (e < minEnd)
                                        minEnd = e;
                                }
                            }
                        }
                    }
                    if (registerOperand(op, 1))
                        cnt -= countSetBits(op->componentMask);
                }
                if (minEnd < 30000 && __builtin_expect(cnt >= 0, 1)) {
                    int t = minEnd;
                    InstructionPosition pos;
                    bool found = false;
                    for (; t > lo; t--) {
                        pos.setFromLinearRow(t);
                        if (!checkPlacementInAllowedPipelines(wt, in, &pos, false)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        if (!wt->takeOut(in)) {
                            reportError("failed to take out instruction\n");
                            abortAfterError();
                        }
                        if (!moveLocks(in, &pos)) {
                            reportError("failed to move !\n");
                            abortAfterError();
                        }
                        if (!wt->putIn(in)) {
                            reportError("failed to put in back in\n");
                            abortAfterError();
                        }
                        moved++;
                    }
                }
            }
        }
    } while (moved);
    return true;
}

int drag(workTable *wt, Instr *in, InstructionPosition *pos, int back, int depth)
{
    if (!wt->loopSize)
        return 0;
    pullSpecialRegisterWritersEarlier(wt, in, pos);
    pullLatencyWritersEarlier(wt, in, pos);
    int total = 0;
    int n;
    do {
        n = 0;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            RegPoolEntry *r = registerOperand(op, 1);
            if (!r)
                continue;
            for (int p = 0; p != r->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                OpSlot *s = &op->parts[p];
                OperandLock *par = s->getParent();
                Instr *oi = par->ownerInstr();
                if (!oi)
                    continue;
                int maxPos = pos->linearRow - par->issueLatency - s->issueLatency;
                int start = pos->linearRow - back;
                if (!back)
                    start = oi->position.linearRow + 1;
                if (!par->latencyModeFlag)
                    start -= par->issueLatency;
                bool single = true;
                if (oi->requiresFullLoopMoveDrag) {
                    start = oi->position.linearRow + 1;
                    if (maxPos > wt->loopSize + oi->position.linearRow)
                        maxPos = wt->loopSize + oi->position.linearRow;
                    if (maxPos > oi->moveRowOffset)
                        maxPos = oi->moveRowOffset;
                    single = false;
                }
                InstructionPosition *opos = &oi->position;
                if (start <= opos->linearRow)
                    continue;
                if (start > maxPos)
                    continue;
                bool moved = false;
                char pipe = opos->pipelineIndex;
                InstructionPosition np;
                np.setInstructionPipe(pipe);
                np.setFromLinearRow(start);
                if (depth <= 3) {
                    int b = back;
                    if (oi->requiresFullLoopMoveDrag)
                        b = wt->loopSize;
                    InstructionPosition tmp;
                    n += drag(wt, oi, &np, b, depth + 1);
                }
                InstructionPosition cur;
                cur.linearRow = np.linearRow;
                cur.loopRow = np.loopRow;
                cur.loopIteration = np.loopIteration;
                cur.pipelineIndex = np.pipelineIndex;
                for (; start <= maxPos; start++) {
                    cur.setFromLinearRow(start);
                    if (!checkPlacementInAllowedPipelines(wt, oi, &cur, false)) {
                        np.linearRow = cur.linearRow;
                        np.loopRow = cur.loopRow;
                        np.loopIteration = cur.loopIteration;
                        np.pipelineIndex = cur.pipelineIndex;
                        moved = true;
                        if (single)
                            break;
                    }
                }
                if (moved) {
                    if (!wt->takeOut(oi)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(oi, &np)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!wt->putIn(oi)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                    n++;
                }
                }
            }
        }
        total += n;
    } while (n);
    return total;
}

RegisterPartLockList *workTable::getLockList(RegPoolEntry *reg, int part)
{
    RegisterPartLockList *ll = findLockList(registerLockLists, reg, part);
    if (ll)
        return ll;
    ll = new RegisterPartLockList(registerLockLists);
    if (!ll) {
        reportError("failed to get new locklist\n");
        abortAfterError();
    }
    registerLockListCount++;
    registerLockLists = ll;
    if (!ll->setRepReg(reg)) {
        reportError("failed to set representative reg\n");
        return 0;
    }
    ll->componentIndex = part;
    return ll;
}

int checkInstructionPlacementConstraints(workTable *wt, Instr *in, InstructionPosition *pos)
{
    int row = pos->linearRow;
    int pipe = pos->pipelineIndex;
    int r4 = pos->loopRow;

    short e = in->externalInputConstraint;
    if (e < in->externalInputPairedConstraint)
        e = in->externalInputPairedConstraint;
    if (r4 < e + activeSchedulingRangeStart)
        return 0x10000;

    short l = in->externalOutputConstraint;
    if (l < in->externalOutputPairedConstraint)
        l = in->externalOutputPairedConstraint;
    if (l && activeSchedulingRangeEnd >= 0 && activeSchedulingRangeEnd - r4 <= l)
        return 0x10000;
    if (staggerMemoryAccess && in->ramAccessType && (bool)(r4 & 1))
        return 0x10000;

    if (wt->loopSize) {
        if (row / wt->loopSize > 16)
            return 0x8000;
    } else if (row >= wt->rowCapacity) {
        return 0x8000;
    }

    if (!in->hasExtraPredecessors) {
        int fr = 0;
        if (!calculateEarliestDependencyRow(in, &fr)) {
            reportError("check fdeps failed\n");
            exitAfterCleanup(-1);
            return 0x80000000;
        }
        if (fr < 0 || fr > row)
            return 0x100000;
    }
    int lr = 30000;
    if (!calculateLatestDependencyRow(in, &lr)) {
        reportError("check fdeps failed\n");
        exitAfterCleanup(-1);
        return 0x80000000;
    }
    if (lr <= row)
        return 0x100000;

    Instr **ie = ((cls_unk30 *)wt)->getIE_ck(pos->loopRow);
    if (ie[pipe] != (Instr *)in->replacedInstructionWord && ie[pipe] != in)
        return 0x40;
    if (!wouldPair(ie, in, pipe))
        return 0x80;
    return canMoveLocks(in, &pos->linearRow);
}

int pullSpecialRegisterWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos)
{
    int row = in->position.getRow() < 0 ? pos->getRow() : in->position.getRow();
    if (row < 0) {
        reportError("inst not inserted\n");
        abortAfterError();
    }
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((op->componentMask >> k) & 1) {
                OpSlot *s = &op->parts[k];
                OperandLock *par = s->getParent();
                int idx = reg->registerIndex;
                if (idx != 48 && idx != 49 && idx != 59)
                    continue;
                Instr *oi = par->ownerInstr();
                if (!oi)
                    continue;
                int t = in->position.linearRow - par->issueLatency - s->issueLatency;
                int oiRow = oi->position.linearRow;
                if (t <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(t);
                for (; oiRow < t; t--) {
                    trial.setFromLinearRow(t);
                    if (!checkPlacementInAllowedPipelines(wt, oi, &trial, true)) {
                        dest.linearRow = trial.linearRow;
                        dest.loopRow = trial.loopRow;
                        dest.loopIteration = trial.loopIteration;
                        dest.pipelineIndex = trial.pipelineIndex;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (!wt->takeOut(oi)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(oi, &dest)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!wt->putIn(oi)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                }
            }
        }
    }
    return 1;
}

int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards)
{
    int result = 0;
    if (in->permittedPipelineMask == 3) {
        if (upwards) {
            for (int i = 0; i != 2; i++) {
                pos->setInstructionPipe(i);
                int r = checkInstructionPlacementConstraints(wt, in, pos);
                if (!r)
                    return 0;
                result |= r;
            }
        } else {
            for (int i = 1; i >= 0; i--) {
                pos->setInstructionPipe(i);
                int r = checkInstructionPlacementConstraints(wt, in, pos);
                if (!r)
                    return 0;
                result |= r;
            }
        }
        return result;
    }
    pos->setInstructionPipe(in->position.getPipe());
    return checkInstructionPlacementConstraints(wt, in, pos);
}

int pullLatencyWritersEarlier(workTable *wt, Instr *in, InstructionPosition *pos)
{
    int row = in->position.getRow() < 0 ? pos->getRow() : in->position.getRow();
    if (row < 0) {
        reportError("inst not inserted\n");
        abortAfterError();
    }
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((op->componentMask >> k) & 1) {
                OpSlot *s = &op->parts[k];
                OperandLock *par = s->getParent();
                if (par->activeReferenceCount != 0)
                    continue;
                Instr *oi = par->ownerInstr();
                if (!oi)
                    continue;
                if (oi->ramAccessType != 1 && oi->schedulingStatistics.forwardLatencyDepth != 0)
                    continue;
                int t = in->position.linearRow - par->issueLatency - s->issueLatency;
                int oiRow = oi->position.linearRow;
                if (t <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(t);
                for (; oiRow < t; t--) {
                    trial.setFromLinearRow(t);
                    if (!checkPlacementInAllowedPipelines(wt, oi, &trial, true)) {
                        dest.linearRow = trial.linearRow;
                        dest.loopRow = trial.loopRow;
                        dest.loopIteration = trial.loopIteration;
                        dest.pipelineIndex = trial.pipelineIndex;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (!wt->takeOut(oi)) {
                        reportError("drag: failed to take in from table\n");
                        abortAfterError();
                    }
                    if (!moveLocks(oi, &dest)) {
                        reportError("failed to move !\n");
                        abortAfterError();
                    }
                    if (!wt->putIn(oi)) {
                        reportError("failed to put in dragged instruction\n");
                        abortAfterError();
                    }
                }
            }
        }
    }
    return 1;
}
