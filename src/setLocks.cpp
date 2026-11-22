#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

class Block;
bool func_0806558c(Instr *i, int *row);
bool func_080655e8(Instr *i, int *row);
bool unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
bool AddDone_Tail(Block *blk, Instr *n);
bool UnlinkFromDone(Block *blk, Instr *n);
bool AddToDo_Head(Block *blk, Instr *n);
bool setRange(int start, int end, int lsz);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int func_0807de00(workTable *wt, Instr *in, InstructionPosition *pos);
int func_0807e1f8(workTable *wt, Instr *in, InstructionPosition *pos);
int func_0804bf50(unsigned int x);
extern bool stagger_080b33ac;

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
                op->parts[p].unk_52[0] = 0;
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
                if (*minPos < ll->unk_00) {
                    ret |= 0x80000;
                    *minPos = ll->unk_00;
                    goto done;
                }
                int rc = ll->checkInputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x430) {
                    OperandLock *par = s->getParent();
                    Operand *o = par->ownerOperand();
                    if (o) {
                        Instr *oi = par->ownerInstr();
                        if (!o->parts[p].unk_52[0]) {
                            o->parts[p].unk_52[0] = 1;
                            o->parts[p].unk_50++;
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
                if (op->parts[p].unk_50) {
                    int v = ll->maxConstraintOfFirstOutputs(op->parts[p].unk_50);
                    if (v > *minPos) {
                        ret |= 0x80000;
                        *minPos = v;
                        goto done;
                    }
                }
                int rc = ll->checkOutputLock(s, pos, minPos);
                ret |= rc;
                if (rc & 0x22000) {
                    if (!g_080b336c) {
                        reportError("late extern, loop overrun, but no loop\n");
                        abortAfterError();
                    }
                    int d = *pos + s->issueLatency - g_080b336c;
                    if (d > 0) {
                        ll->unk_00 = d;
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
    d->unk_00 = pos->unk_00;
    d->unk_04 = pos->unk_04;
    d->unk_03 = pos->unk_03;
    d->unk_02 = pos->unk_02;
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
            op->assignedRegister->unk_34 = 0;
    }
    for (i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        RegPoolEntry *r = op->inputRegister();
        if (!r)
            continue;
        for (int p = 0; p != r->partCount; p++) {
            if ((op->componentMask >> p) & 1) {
                OpSlot *s = &op->parts[p];
                s->unk_01 = 0;
                s->unk_00 = 0;
                s->unk_02 = 0;
                if (!(((unsigned char)r->unk_34 >> p) & 1)) {
                    switch ((unsigned char)in->permittedPipelineMask) {
                    case 0:
                        reportError("no physical pipe bits!\n");
                        return false;
                    case 1:
                        s->unk_01 = 1;
                        break;
                    case 2:
                        s->unk_00 = 1;
                        break;
                    case 3:
                        s->unk_02 = 1;
                        break;
                    default:
                        reportError("unknown pipe request\n");
                        return false;
                    }
                }
            }
        }
        r->unk_34 |= op->componentMask;
    }
    return true;
}

int checkInsert(workTable *wt, Instr *in, InstructionPosition *pos, int *minPos)
{
    int row = pos->unk_00;
    int pipe = pos->unk_02;
    int r4 = pos->unk_04;

    if (*minPos < row)
        *minPos = row;

    short e = in->unk_13b2;
    if (e < in->unk_13b0)
        e = in->unk_13b0;
    int v = e;
    if (r4 < v + g_080b3370) {
        int m = *minPos;
        int next;
        if (!g_080b336c)
            next = v;
        else {
            int rem = v % g_080b336c;
            int q = m / g_080b336c;
            if (rem <= m % g_080b336c)
                q++;
            next = q * g_080b336c + rem;
        }
        *minPos = next;
        return 0x10000;
    }

    short l = in->unk_13b6;
    if (l < in->unk_13b4)
        l = in->unk_13b4;
    if (l && g_080b3374 >= 0 && g_080b3374 - r4 <= l) {
        *minPos = 30000;
        return 0x10000;
    }
    if (stagger_080b33ac && in->ramAccessType && (bool)(r4 & 1)) {
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
        if (!func_0806558c(in, &fr)) {
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

    Instr **ie = ((cls_unk30 *)wt)->getIE_ck(pos->unk_04);
    if (ie[pipe] != (Instr *)in->unk_1398) {
        *minPos = row + 1;
        return 0x40;
    }
    if (!wouldPair(ie, in, pipe)) {
        (*minPos)++;
        return 0x80;
    }
    return checkLocks(in, &pos->unk_00, minPos);
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
        int zp = (unsigned char)pos->unk_03;
        zp >>= 4;
        zp >>= 4;
        if (zp)
            return r != p;
    }
    r = pos->unk_04;
    wtRow *row = &wt->rows[r];
    InstructionPosition *s = &in->position;
    b = pos->unk_00;
    s->unk_04 = r;
    char a = pos->unk_03;
    s->unk_03 = a;
    p = pos->getPipe();
    s->unk_00 = b;
    s->unk_02 = p;
    row->entries[p].unk_00 = in;
    b = setLocks(in);
    if (!(char)b) {
        reportError("DoInsert: failed to set locks in instruction insert\n");
        return false;
    }
    int zt;
    do {
        in->insertionSerial = wt->unk_40;
        wt->unk_40++;
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
        zt = ((unsigned char)pos->unk_02 | (unsigned char)pos->unk_03);
        zt >>= 4;
        zt >>= 4;
    } while (zt);
    return true;
}

bool UndoInsert(workTable *wt, Instr *in)
{
    if (in->branchFlags)
        wt->unk_84 = 0;
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
    wt->unk_40--;
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
                int lo = in->position.unk_00;
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
                                    int e = s->unk_3e;
                                    if (e < minEnd)
                                        minEnd = e;
                                }
                            }
                        }
                    }
                    if (registerOperand(op, 1))
                        cnt -= func_0804bf50(op->componentMask);
                }
                if (minEnd < 30000 && __builtin_expect(cnt >= 0, 1)) {
                    int t = minEnd;
                    InstructionPosition pos;
                    bool found = false;
                    for (; t > lo; t--) {
                        pos.setFromLinearRow(t);
                        if (!func_0807e130(wt, in, &pos, false)) {
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
    func_0807de00(wt, in, pos);
    func_0807e1f8(wt, in, pos);
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
                int maxPos = pos->unk_00 - par->issueLatency - s->issueLatency;
                int start = pos->unk_00 - back;
                if (!back)
                    start = oi->position.unk_00 + 1;
                if (!par->unk_15)
                    start -= par->issueLatency;
                bool single = true;
                if (oi->unk_13d1) {
                    start = oi->position.unk_00 + 1;
                    if (maxPos > wt->loopSize + oi->position.unk_00)
                        maxPos = wt->loopSize + oi->position.unk_00;
                    if (maxPos > oi->moveRowOffset)
                        maxPos = oi->moveRowOffset;
                    single = false;
                }
                InstructionPosition *opos = &oi->position;
                if (start <= opos->unk_00)
                    continue;
                if (start > maxPos)
                    continue;
                bool moved = false;
                char pipe = opos->unk_02;
                InstructionPosition np;
                np.setInstructionPipe(pipe);
                np.setFromLinearRow(start);
                if (depth <= 3) {
                    int b = back;
                    if (oi->unk_13d1)
                        b = wt->loopSize;
                    InstructionPosition tmp;
                    n += drag(wt, oi, &np, b, depth + 1);
                }
                InstructionPosition cur;
                cur.unk_00 = np.unk_00;
                cur.unk_04 = np.unk_04;
                cur.unk_03 = np.unk_03;
                cur.unk_02 = np.unk_02;
                for (; start <= maxPos; start++) {
                    cur.setFromLinearRow(start);
                    if (!func_0807e130(wt, oi, &cur, false)) {
                        np.unk_00 = cur.unk_00;
                        np.unk_04 = cur.unk_04;
                        np.unk_03 = cur.unk_03;
                        np.unk_02 = cur.unk_02;
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
    unk_20++;
    registerLockLists = ll;
    if (!ll->setRepReg(reg)) {
        reportError("failed to set representative reg\n");
        return 0;
    }
    ll->componentIndex = part;
    return ll;
}

int func_0807dc44(workTable *wt, Instr *in, InstructionPosition *pos)
{
    int row = pos->unk_00;
    int pipe = pos->unk_02;
    int r4 = pos->unk_04;

    short e = in->unk_13b2;
    if (e < in->unk_13b0)
        e = in->unk_13b0;
    if (r4 < e + g_080b3370)
        return 0x10000;

    short l = in->unk_13b6;
    if (l < in->unk_13b4)
        l = in->unk_13b4;
    if (l && g_080b3374 >= 0 && g_080b3374 - r4 <= l)
        return 0x10000;
    if (stagger_080b33ac && in->ramAccessType && (bool)(r4 & 1))
        return 0x10000;

    if (wt->loopSize) {
        if (row / wt->loopSize > 16)
            return 0x8000;
    } else if (row >= wt->rowCapacity) {
        return 0x8000;
    }

    if (!in->hasExtraPredecessors) {
        int fr = 0;
        if (!func_0806558c(in, &fr)) {
            reportError("check fdeps failed\n");
            exitAfterCleanup(-1);
            return 0x80000000;
        }
        if (fr < 0 || fr > row)
            return 0x100000;
    }
    int lr = 30000;
    if (!func_080655e8(in, &lr)) {
        reportError("check fdeps failed\n");
        exitAfterCleanup(-1);
        return 0x80000000;
    }
    if (lr <= row)
        return 0x100000;

    Instr **ie = ((cls_unk30 *)wt)->getIE_ck(pos->unk_04);
    if (ie[pipe] != (Instr *)in->unk_1398 && ie[pipe] != in)
        return 0x40;
    if (!wouldPair(ie, in, pipe))
        return 0x80;
    return canMoveLocks(in, &pos->unk_00);
}

int func_0807de00(workTable *wt, Instr *in, InstructionPosition *pos)
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
                int t = in->position.unk_00 - par->issueLatency - s->issueLatency;
                int oiRow = oi->position.unk_00;
                if (t <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(t);
                for (; oiRow < t; t--) {
                    trial.setFromLinearRow(t);
                    if (!func_0807e130(wt, oi, &trial, true)) {
                        dest.unk_00 = trial.unk_00;
                        dest.unk_04 = trial.unk_04;
                        dest.unk_03 = trial.unk_03;
                        dest.unk_02 = trial.unk_02;
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

int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards)
{
    int result = 0;
    if (in->permittedPipelineMask == 3) {
        if (upwards) {
            for (int i = 0; i != 2; i++) {
                pos->setInstructionPipe(i);
                int r = func_0807dc44(wt, in, pos);
                if (!r)
                    return 0;
                result |= r;
            }
        } else {
            for (int i = 1; i >= 0; i--) {
                pos->setInstructionPipe(i);
                int r = func_0807dc44(wt, in, pos);
                if (!r)
                    return 0;
                result |= r;
            }
        }
        return result;
    }
    pos->setInstructionPipe(in->position.getPipe());
    return func_0807dc44(wt, in, pos);
}

int func_0807e1f8(workTable *wt, Instr *in, InstructionPosition *pos)
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
                if (par->unk_22 != 0)
                    continue;
                Instr *oi = par->ownerInstr();
                if (!oi)
                    continue;
                if (oi->ramAccessType != 1 && oi->unk_13f0.unk_04 != 0)
                    continue;
                int t = in->position.unk_00 - par->issueLatency - s->issueLatency;
                int oiRow = oi->position.unk_00;
                if (t <= oiRow)
                    continue;
                bool found = false;
                InstructionPosition trial;
                InstructionPosition dest;
                dest.setFromLinearRow(t);
                for (; oiRow < t; t--) {
                    trial.setFromLinearRow(t);
                    if (!func_0807e130(wt, oi, &trial, true)) {
                        dest.unk_00 = trial.unk_00;
                        dest.unk_04 = trial.unk_04;
                        dest.unk_03 = trial.unk_03;
                        dest.unk_02 = trial.unk_02;
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
