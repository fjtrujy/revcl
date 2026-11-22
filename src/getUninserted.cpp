#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

int getToEnd(Instr *in);
int getFromStart(Instr *in);

instStream *selectUninsertedStream(Program *p)
{
    instStream *s = p->streamHead;
    instStream *best = 0;
    int minIn = 1000000;
    int maxOut = -1000000;
    for (; s; s = s->nextStream) {
        if (s->getCodeMode() == 1) {
            int in = s->getUninsertedxInputs();
            int out = s->getUninsertedxOutputs();
            if (in < 0) {
                reportError("failed to get uninserted inputs\n");
                return 0;
            }
            if (out < 0) {
                reportError("failed to get uninserted outputs\n");
                return 0;
            }
            if (in < minIn) {
                best = s;
                minIn = in;
                maxOut = out;
            } else if (in == minIn && out > maxOut) {
                best = s;
                maxOut = out;
            }
        }
    }
    return best;
}

int instStream::getUninsertedxInputs()
{
    int count = 0;
    if (!workTablePtr) {
        reportError("work table not set\n");
        return -1;
    }
    for (Instr *in = workTablePtr->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        int base = getFromStart(in);
        if (base < 0) {
            reportError("iStream::: getUninsertedxInputs - failed\n");
            return -1;
        }
        base += unk_278.unk_00 - in->unk_13f0.unk_00;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(op, 1);
            if (!reg || !op->externalPartMask)
                continue;
            for (int k = 0; k != reg->partCount; k++) {
                if ((op->externalPartMask >> k) & 1) {
                ol_s *ol = op->parts[k].unk_54.head;
                if (!ol) {
                    reportError("no setter list found during getuninstxi!\n");
                    return -1;
                }
                for (; ol; ol = ol->next) {
                    Instr *setter = ol->instruction;
                    if (setter->getOwningStream() == this || setter->unk_00)
                        continue;
                    int te = getToEnd(setter);
                    if (te < 0) {
                        reportError("getUninsertedxInputs - failed to get to end for a setter\n");
                        return -1;
                    }
                    Operand *src = operandAtLocation(ol);
                    if (te + base + ol->unk_12 + 1 < src->outputLatency + op->additionalLatency) {
                        if (reg->partCount == 1)
                            count += 4;
                        else
                            count++;
                    }
                }
                }
            }
        }
    }
    return count;
}

int instStream::getUninsertedxOutputs()
{
    int count = 0;
    if (!workTablePtr) {
        reportError("work table not set\n");
        return -1;
    }
    for (Instr *in = workTablePtr->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        int base = getToEnd(in);
        if (base < 0) {
            reportError("iStream::: getUninsertedxOutputs - failed\n");
            return -1;
        }
        base += unk_278.unk_04 - in->unk_13f0.unk_04;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(op, 2);
            if (!reg || !op->externalPartMask)
                continue;
            for (int k = 0; k != reg->partCount; k++) {
                if ((op->externalPartMask >> k) & 1) {
                    for (ol_s *ol = op->parts[k].unk_54.head; ol; ol = ol->next) {
                        Instr *user = ol->instruction;
                        if (user->getOwningStream() == this || user->unk_00)
                            continue;
                        int fs = getFromStart(user);
                        if (fs < 0) {
                            reportError("getUninsertedxOutputs - failed to get to end for a setter\n");
                            return -1;
                        }
                        Operand *src = operandAtLocation(ol);
                        if (base + fs + ol->unk_12 + 1 < op->outputLatency + src->additionalLatency) {
                            if (reg->partCount == 1)
                                count += 4;
                            else
                                count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

int getToEnd(Instr *in)
{
    workTable *wt = (workTable *)in->unk_30;
    if (!wt) {
        reportError("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *s = wt->ownerStream;
    if (!s) {
        reportError("failed to get owner owner\n");
        return -1;
    }
    Instr *img = in;
    if (in->unk_00) {
        img = (Instr *)in->unk_00;
        if (!img->unk_30) {
            reportError("image had no owner\n");
            return -1;
        }
        if (s != img->getOwningStream()) {
            reportError("our image was in different stream\n");
            return -1;
        }
        wt = (workTable *)img->unk_30;
        if (wt->codeModeValue != 3) {
            reportError("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (wt->codeModeValue) {
    case 1:
        return in->unk_13f0.unk_00;
    case 3: {
        int len = wt->func_08072eec();
        if (len < 0) {
            reportError("logical len<0 for table \n");
            return -1;
        }
        int et = img->scheduledCycle;
        if (et < 0) {
            reportError("exec time not set\n");
            return -1;
        }
        int te = len - et - 1;
        if (te < 0) {
            reportError("te<0... for getLLen.. eTim=%d tSiz=%d\n", et, len, te);
            wt->dump();
            reportError("instruction is\n");
            img->func_08054008(false);
            break;
        }
        return te;
    }
    case 4:
        return 0;
    default:
        const char *modeName = codeModeName(wt->codeModeValue);
        reportError("bad code mode for getToEnd()..mode=%s\n", modeName);
        break;
    }
    return -1;
}

int getFromStart(Instr *in)
{
    workTable *wt = (workTable *)in->unk_30;
    if (!wt) {
        reportError("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *s = wt->ownerStream;
    if (!s) {
        reportError("failed to get owner owner\n");
        return -1;
    }
    Instr *img = in;
    if (in->unk_00) {
        img = (Instr *)in->unk_00;
        if (!img->unk_30) {
            reportError("image had no owner\n");
            return -1;
        }
        if (s != img->getOwningStream()) {
            reportError("our image was in different stream\n");
            return -1;
        }
        wt = (workTable *)img->unk_30;
        if (wt->codeModeValue != 3) {
            reportError("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (wt->codeModeValue) {
    case 1:
        return in->unk_13f0.unk_04;
    case 3: {
        int len = wt->func_08072eec();
        if (len < 0) {
            reportError("logical len<0 for table \n");
            return -1;
        }
        int et = img->scheduledCycle;
        if (et < 0) {
            reportError("exec time not set\n");
            return -1;
        }
        if (et < len)
            return et;
        reportError("tick after table end\n");
        return -1;
    }
    case 4:
        return 0;
    default:
        reportError("bad code mode for getFromStart()\n");
        break;
    }
    return -1;
}

bool optReset(Program *p);
bool setRange(int start, int end, int lsz);

bool loadLocks(Program *p, workTable *wt)
{
    if (!wt) {
        reportError("load locks to registers - failed\n");
        return false;
    }
    if (!wt->unk_0b) {
        reportError("locks aren't valid for loadlocks\n");
        return false;
    }
    if (!optReset(p)) {
        reportError("failed to opt reset\n");
        return false;
    }
    setRange(wt->getRangeStart(), wt->getRangeEnd(), wt->getLoopSize());
    for (regUse *ru = wt->registerUses.head; ru; ru = ru->next) {
        RegPoolEntry *reg = ru->registerEntry;
        for (int i = 0; i != reg->partCount; i++) {
            regUseSlot *rs = &ru->slots[i];
            if (!rs) {
                reportError("failed to get rrp\n");
                exitAfterCleanup(-1);
            }
            RegisterPartLockList *rll = (RegisterPartLockList *)rs->assignmentPayload;
            if (!rll) {
                reportError("no rll set in regref\n");
                exitAfterCleanup(-1);
            }
            RegisterPartLockList *cur = (RegisterPartLockList *)reg->getPartPayload(i);
            if (!cur)
                reg->setPartPayload(i, (VclPtrWord)rll);
            else if (rll != cur) {
                reportError("rll mismatch\n");
                exitAfterCleanup(-1);
            }
        }
    }
    return true;
}
