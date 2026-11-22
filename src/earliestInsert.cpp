#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"
#include "ol_s_inline.h"

int earliestInsert(workTable *wt, Instr *in, bool *found)
{
    short e = in->externalInputConstraint;
    if (e < in->externalInputPairedConstraint)
        e = in->externalInputPairedConstraint;
    int line = e + wt->rangeStart;
    int thru = -1;
    int iline = in->position.loopRow;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (reg) {
            for (int p = 0; p != reg->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *slot = &op->parts[p];
                    OperandLock *par = slot->getParent();
                    if (slot->operandStatusCode != 2) {
                        int s = par->linearRow;
                        if ((short)s >= 0) {
                            int t = s + par->issueLatency + slot->issueLatency;
                            if (t >= line) {
                                line = t;
                                if (op->externalBlockModeFlag && !op->additionalLatency)
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
        if (reg && reg->tracksHotOutputPaths) {
            for (int p = 0; p != reg->partCount; p++) {
                if ((op->componentMask >> p) & 1) {
                    OpSlot *slot = &op->parts[p];
                    OperandLock *lk = slot->ownerLockList->sortedOutputLocks;
                    while (lk && lk != slot) {
                        int s = lk->linearRow;
                        if (s > iline) {
                            reportError("sorted list corrupted?\n");
                            exitAfterCleanup(-1);
                        }
                        if (s >= 0) {
                            int t = s + lk->throughputSpacing;
                            if (t >= line)
                                line = thru = t;
                            if (t > iline) {
                                reportError("ln>iline for thru case\n");
                                exitAfterCleanup(-1);
                            }
                        }
                        lk = lk->nextSortedOutput;
                    }
                    if (!lk) {
                        reportError("sorted lock not  on list for get earliest!\n");
                        exitAfterCleanup(-1);
                    }
                }
            }
        }
        if (registerOperand(op, 2) && op->reductionHazardMask)
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
    short e = in->externalOutputConstraint;
    if (e < in->externalOutputPairedConstraint)
        e = in->externalOutputPairedConstraint;
    int line = wt->rangeEnd - e - 1;
    int iline = in->position.loopRow;
    bool thru = false;
    Operand *op;
    for (int i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(op, 1);
        if (reg && op->reductionHazardMask) {
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
