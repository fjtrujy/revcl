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
    instStream *s = p->unk_a0;
    instStream *best = 0;
    int minIn = 1000000;
    int maxOut = -1000000;
    for (; s; s = s->unk_108) {
        if (s->func_0806d908() == 1) {
            int in = s->getUninsertedxInputs();
            int out = s->getUninsertedxOutputs();
            if (in < 0) {
                func_0804ad38("failed to get uninserted inputs\n");
                return 0;
            }
            if (out < 0) {
                func_0804ad38("failed to get uninserted outputs\n");
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
    if (!unk_218) {
        func_0804ad38("work table not set\n");
        return -1;
    }
    for (Instr *in = unk_218->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int base = getFromStart(in);
        if (base < 0) {
            func_0804ad38("iStream::: getUninsertedxInputs - failed\n");
            return -1;
        }
        base += unk_278.unk_00 - in->unk_13f0.unk_00;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(op, 1);
            if (!reg || !op->unk_27)
                continue;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((op->unk_27 >> k) & 1) {
                ol_s *ol = op->unk_2c[k].unk_54.unk_00;
                if (!ol) {
                    func_0804ad38("no setter list found during getuninstxi!\n");
                    return -1;
                }
                for (; ol; ol = ol->unk_00) {
                    Instr *setter = ol->unk_04;
                    if (setter->func_08054168() == this || setter->unk_00)
                        continue;
                    int te = getToEnd(setter);
                    if (te < 0) {
                        func_0804ad38("getUninsertedxInputs - failed to get to end for a setter\n");
                        return -1;
                    }
                    Operand *src = func_0805802c(ol);
                    if (te + base + ol->unk_12 + 1 < src->unk_0a + op->unk_29) {
                        if (reg->unk_28 == 1)
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
    if (!unk_218) {
        func_0804ad38("work table not set\n");
        return -1;
    }
    for (Instr *in = unk_218->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int base = getToEnd(in);
        if (base < 0) {
            func_0804ad38("iStream::: getUninsertedxOutputs - failed\n");
            return -1;
        }
        base += unk_278.unk_04 - in->unk_13f0.unk_04;
        Operand *op;
        for (int i = 0; (op = in->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(op, 2);
            if (!reg || !op->unk_27)
                continue;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((op->unk_27 >> k) & 1) {
                    for (ol_s *ol = op->unk_2c[k].unk_54.unk_00; ol; ol = ol->unk_00) {
                        Instr *user = ol->unk_04;
                        if (user->func_08054168() == this || user->unk_00)
                            continue;
                        int fs = getFromStart(user);
                        if (fs < 0) {
                            func_0804ad38("getUninsertedxOutputs - failed to get to end for a setter\n");
                            return -1;
                        }
                        Operand *src = func_0805802c(ol);
                        if (base + fs + ol->unk_12 + 1 < op->unk_0a + src->unk_29) {
                            if (reg->unk_28 == 1)
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
        func_0804ad38("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *s = wt->unk_10;
    if (!s) {
        func_0804ad38("failed to get owner owner\n");
        return -1;
    }
    Instr *img = in;
    if (in->unk_00) {
        img = (Instr *)in->unk_00;
        if (!img->unk_30) {
            func_0804ad38("image had no owner\n");
            return -1;
        }
        if (s != img->func_08054168()) {
            func_0804ad38("our image was in different stream\n");
            return -1;
        }
        wt = (workTable *)img->unk_30;
        if (wt->unk_09 != 3) {
            func_0804ad38("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (wt->unk_09) {
    case 1:
        return in->unk_13f0.unk_00;
    case 3: {
        int len = wt->func_08072eec();
        if (len < 0) {
            func_0804ad38("logical len<0 for table \n");
            return -1;
        }
        int et = img->unk_1394;
        if (et < 0) {
            func_0804ad38("exec time not set\n");
            return -1;
        }
        int te = len - et - 1;
        if (te < 0) {
            func_0804ad38("te<0... for getLLen.. eTim=%d tSiz=%d\n", et, len, te);
            wt->dump();
            func_0804ad38("instruction is\n");
            img->func_08054008(false);
            break;
        }
        return te;
    }
    case 4:
        return 0;
    default:
        const char *modeName = codeModeName(wt->unk_09);
        func_0804ad38("bad code mode for getToEnd()..mode=%s\n", modeName);
        break;
    }
    return -1;
}

int getFromStart(Instr *in)
{
    workTable *wt = (workTable *)in->unk_30;
    if (!wt) {
        func_0804ad38("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *s = wt->unk_10;
    if (!s) {
        func_0804ad38("failed to get owner owner\n");
        return -1;
    }
    Instr *img = in;
    if (in->unk_00) {
        img = (Instr *)in->unk_00;
        if (!img->unk_30) {
            func_0804ad38("image had no owner\n");
            return -1;
        }
        if (s != img->func_08054168()) {
            func_0804ad38("our image was in different stream\n");
            return -1;
        }
        wt = (workTable *)img->unk_30;
        if (wt->unk_09 != 3) {
            func_0804ad38("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (wt->unk_09) {
    case 1:
        return in->unk_13f0.unk_04;
    case 3: {
        int len = wt->func_08072eec();
        if (len < 0) {
            func_0804ad38("logical len<0 for table \n");
            return -1;
        }
        int et = img->unk_1394;
        if (et < 0) {
            func_0804ad38("exec time not set\n");
            return -1;
        }
        if (et < len)
            return et;
        func_0804ad38("tick after table end\n");
        return -1;
    }
    case 4:
        return 0;
    default:
        func_0804ad38("bad code mode for getFromStart()\n");
        break;
    }
    return -1;
}

bool optReset(Program *p);
bool setRange(int start, int end, int lsz);

bool loadLocks(Program *p, workTable *wt)
{
    if (!wt) {
        func_0804ad38("load locks to registers - failed\n");
        return false;
    }
    if (!wt->unk_0b) {
        func_0804ad38("locks aren't valid for loadlocks\n");
        return false;
    }
    if (!optReset(p)) {
        func_0804ad38("failed to opt reset\n");
        return false;
    }
    setRange(wt->func_0807411c(), wt->func_080741c0(), wt->func_080741b4());
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        for (int i = 0; i != reg->unk_28; i++) {
            regUseSlot *rs = &ru->unk_20[i];
            if (!rs) {
                func_0804ad38("failed to get rrp\n");
                func_0804adf0(-1);
            }
            cls0805c244 *rll = (cls0805c244 *)rs->unk_58;
            if (!rll) {
                func_0804ad38("no rll set in regref\n");
                func_0804adf0(-1);
            }
            cls0805c244 *cur = (cls0805c244 *)reg->func_0805a908(i);
            if (!cur)
                reg->func_0805a924(i, (VclPtrWord)rll);
            else if (rll != cur) {
                func_0804ad38("rll mismatch\n");
                func_0804adf0(-1);
            }
        }
    }
    return true;
}
