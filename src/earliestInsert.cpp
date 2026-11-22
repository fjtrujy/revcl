#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"
#include "ol_s_inline.h"

int earliestInsert(workTable *wt, Instr *in, bool *found)
{
    short e = in->unk_13b2;
    if (e < in->unk_13b0)
        e = in->unk_13b0;
    int line = e + wt->unk_30;
    int thru = -1;
    int iline = in->position.unk_04;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (reg) {
            for (int p = 0; p != reg->partCount; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *slot = &op->unk_2c[p];
                    OperandLock *par = slot->getParent();
                    if (slot->unk_19 != 2) {
                        int s = par->unk_32;
                        if ((short)s >= 0) {
                            int t = s + par->unk_18 + slot->unk_18;
                            if (t >= line) {
                                line = t;
                                if (op->unk_0b && !op->unk_29)
                                    thru = t;
                            }
                            if (t > iline) {
                                reportError("ln>iline.. \n");
                                slot->print();
                                exitAfterCleanup(-1);
                            }
                        }
                    }
                }
            }
        }
        reg = registerOperand(op, 2);
        if (reg && reg->unk_2c) {
            for (int p = 0; p != reg->partCount; p++) {
                if ((op->unk_07 >> p) & 1) {
                    OpSlot *slot = &op->unk_2c[p];
                    OperandLock *lk = slot->unk_04->unk_20;
                    while (lk && lk != slot) {
                        int s = lk->unk_32;
                        if (s > iline) {
                            reportError("sorted list corrupted?\n");
                            exitAfterCleanup(-1);
                        }
                        if (s >= 0) {
                            int t = s + lk->unk_17;
                            if (t >= line)
                                line = thru = t;
                            if (t > iline) {
                                reportError("ln>iline for thru case\n");
                                exitAfterCleanup(-1);
                            }
                        }
                        lk = lk->unk_10;
                    }
                    if (!lk) {
                        reportError("sorted lock not  on list for get earliest!\n");
                        exitAfterCleanup(-1);
                    }
                }
            }
        }
        if (registerOperand(op, 2) && op->unk_28)
            line = iline;
    }
    if (in->branchFlags)
        line = iline;
    if (found)
        *found = thru == line;
    return line;
}

int latestInsert(workTable *wt, Instr *in, bool *found)
{
    short e = in->unk_13b6;
    if (e < in->unk_13b4)
        e = in->unk_13b4;
    int line = wt->unk_34 - e - 1;
    int iline = in->position.unk_04;
    bool thru = false;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (reg && op->unk_28) {
            thru = true;
            break;
        }
    }
    if (thru || in->branchFlags)
        line = iline;
    if (found)
        *found = false;
    return line;
}
