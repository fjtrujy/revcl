#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"
#include "Instr.h"
#include "ol_s_inline.h"

int earliestInsert(workTable *scheduleTable, Instr *instruction, bool *found)
{
    short e = instruction->externalInputConstraint;
    if (e < instruction->externalInputPairedConstraint)
        e = instruction->externalInputPairedConstraint;
    int line = e + scheduleTable->rangeStart;
    int thru = -1;
    int iline = instruction->position.loopRow;
    Operand *operand;
    for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (reg) {
            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                if ((operand->componentMask >> partIndex) & 1) {
                    OpSlot *slot = &operand->parts[partIndex];
                    OperandLock *par = slot->getParent();
                    if (slot->operandStatusCode != 2) {
                        int s = par->linearRow;
                        if ((short)s >= 0) {
                            int t = s + par->issueLatency + slot->issueLatency;
                            if (t >= line) {
                                line = t;
                                if (operand->externalBlockModeFlag && !operand->additionalLatency)
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
        reg = registerOperand(operand, 2);
        if (reg && reg->tracksHotOutputPaths) {
            for (int p = 0; p != reg->partCount; p++) {
                if ((operand->componentMask >> p) & 1) {
                    OpSlot *slot = &operand->parts[p];
                    OperandLock *operandLock = slot->ownerLockList->sortedOutputLocks;
                    while (operandLock && operandLock != slot) {
                        int s = operandLock->linearRow;
                        if (s > iline) {
                            reportError("sorted list corrupted?\n");
                            exitAfterCleanup(-1);
                        }
                        if (s >= 0) {
                            int t = s + operandLock->throughputSpacing;
                            if (t >= line)
                                line = thru = t;
                            if (t > iline) {
                                reportError("ln>iline for thru case\n");
                                exitAfterCleanup(-1);
                            }
                        }
                        operandLock = operandLock->nextSortedOutput;
                    }
                    if (!operandLock) {
                        reportError("sorted lock not  on list for get earliest!\n");
                        exitAfterCleanup(-1);
                    }
                }
            }
        }
        if (registerOperand(operand, 2) && operand->reductionHazardMask)
            line = iline;
    }
    if (instruction->branchFlags)
        line = iline;
    if (found)
        *found = thru == line;
    return line;
}

int latestInsert(workTable *scheduleTable, Instr *instruction, bool *found)
{
    short e = instruction->externalOutputConstraint;
    if (e < instruction->externalOutputPairedConstraint)
        e = instruction->externalOutputPairedConstraint;
    int line = scheduleTable->rangeEnd - e - 1;
    int iline = instruction->position.loopRow;
    bool thru = false;
    Operand *operand;
    for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (reg && operand->reductionHazardMask) {
            thru = true;
            break;
        }
    }
    if (thru || instruction->branchFlags)
        line = iline;
    if (found)
        *found = false;
    return line;
}
