#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

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
            RegPoolEntry *reg = op->inputReg();
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
                    if (base + te + ol->unk_12 + 1 < src->unk_0a + op->unk_29) {
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
