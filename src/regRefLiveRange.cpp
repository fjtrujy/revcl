#include "os.h"
#include "genRegPool_shared.h"

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *getRegisterPoolByIndex(unsigned t);
bool resetProgramAllocationsForFirstPass(Program *p);
bool resetProgramAllocationsForSecondPass(Program *p);
bool setRange(int start, int end, int lsz);
int countSetBits(unsigned int);
int setRangeFrom(ScheduleTable *t);


static inline bool validPipe(int pipeIndex)
{
    return pipeIndex == 0 || pipeIndex == 1;
}

bool regRefLiveRange(RegisterUse *registerUse, int part, int *rangeStartOut, int *rangeEndOut, bool *always)
{
    *rangeStartOut = -4;
    *rangeEndOut = -3;
    *always = false;
    regUseSlot *registerUseSlot = &registerUse->slots[part];
    if ((char)bitSet(registerUse->inputMask, part)) {
        if (registerUseSlot->timingValues[0] < 0 || !validPipe(registerUseSlot->timingValues[1]) ||
            registerUseSlot->timingValues[3] < 0 || !validPipe(registerUseSlot->timingValues[4])) {
            reportError("firstInput<0.. but iBit set\n");
            exitAfterCleanup(-1);
        }
    }
    if ((char)bitSet(registerUse->outputMask, part)) {
        if (registerUseSlot->timingValues[5] < 0 || !validPipe(registerUseSlot->timingValues[6]) ||
            registerUseSlot->timingValues[8] < 0 || !validPipe(registerUseSlot->timingValues[9])) {
            reportError("firstInput<0.. but iBit set\n");
            exitAfterCleanup(-1);
        }
    }
    if ((char)bitSet(registerUse->throughputMask & ~(registerUse->outputMask | registerUse->inputMask), part)) {
        *rangeStartOut = -1;
        *rangeEndOut = 30000;
        *always = true;
        return true;
    }
    if ((char)bitSet(registerUse->externalInputMask, part)) {
        *rangeStartOut = -1;
    } else {
        if ((char)bitSet(registerUse->inputMask, part) && registerUseSlot->timingValues[0] <= registerUseSlot->timingValues[5]) {
            reportError("first input before first output.. but not xi %d,%d!\n",
                          registerUseSlot->timingValues[0], registerUseSlot->timingValues[5]);
            exitAfterCleanup(-1);
        }
        if ((char)bitSet(registerUse->outputMask, part))
            *rangeStartOut = registerUseSlot->timingValues[5];
    }
    if ((char)bitSet(registerUse->throughputMask | registerUse->externalOutputMask, part)) {
        *rangeEndOut = 30000;
    } else {
        if ((char)bitSet(registerUse->outputMask, part))
            *rangeEndOut = registerUseSlot->timingValues[8] + 1;
        if ((char)bitSet(registerUse->inputMask, part) && registerUseSlot->timingValues[3] > *rangeEndOut)
            *rangeEndOut = registerUseSlot->timingValues[3];
    }
    int row = *rangeStartOut;
    if (*rangeEndOut <= row) {
        row = *rangeEndOut;
        reportError("liverange for regref has e<=s .. %d,%d?\n", *rangeStartOut, row);
        return false;
    }
    return true;
}

static inline bool isInternalLock(OperandLock *operandLock)
{
    return operandLock->ownerType == 2;
}

static inline bool isInternalIn(OperandLock *operandLock)
{
    return operandLock->operandStatusCode == 2;
}

bool lockLiveRange(RegisterPartLockList *registerPartLocks, OperandLock *outputLock, int *rangeStartOut, int *rangeEndOut)
{
    *rangeStartOut = -1;
    *rangeEndOut = -1;
    if (!isInternalLock(outputLock)) {
        Instr *ins = outputLock->ownerInstr();
        if (!ins) {
            reportError("failed to get instruction for non -internal input\n");
            return false;
        }
        InstructionPosition *instructionPosition = &ins->position;
        int line = instructionPosition->getRow();
        int want = outputLock->linearRow;
        if ((short)line != (short)want) {
            reportError("rLine mismatch\n");
            return false;
        }
        if (!validPipe(instructionPosition->getPipe())) {
            reportError("bad pipe for instruction\n");
            return false;
        }
        *rangeStartOut = line;
    } else {
        *rangeStartOut = -1;
    }
    if (outputLock->externalOutputSlotPlusOne) {
        if (activeLoopSize && *rangeStartOut >= 0)
            *rangeEndOut = *rangeStartOut + activeLoopSize - *rangeStartOut % activeLoopSize;
        else
            *rangeEndOut = 30000;
    } else {
        *rangeEndOut = -1;
    }
    for (OperandLock *operandLock = outputLock->parentOrChildren; operandLock; operandLock = operandLock->nextSibling) {
        if (isInternalIn(operandLock))
            continue;
        Instr *lockInstruction = operandLock->ownerInstr();
        if (!lockInstruction) {
            reportError("no inst for rl\n");
            return false;
        }
        InstructionPosition *lockPosition = &lockInstruction->position;
        int line = lockPosition->getRow();
        int want = operandLock->linearRow;
        if ((short)line != (short)want) {
            reportError("rline mismatch for input locks\n");
            return false;
        }
        if (!validPipe(lockPosition->getPipe())) {
            reportError("bad pipe for instruction\n");
            return false;
        }
        if ((short)line <= *rangeStartOut) {
            reportError("el<=s, but can't age\n");
            return false;
        }
        if ((short)line > *rangeEndOut)
            *rangeEndOut = (short)line;
    }
    if (*rangeEndOut < 0)
        *rangeEndOut = *rangeStartOut + 1;
    return true;
}

bool haveSimultaneousRegisterOutputs(ScheduleTable *scheduleTable, RegisterEntry *registerEntry, RegisterEntry *alternateRegister)
{
    if (scheduleTable->codeModeValue != VCL_CODE_RAW) {
        reportError("hack_slo - called - but table not raw\n");
        return false;
    }
    if (!scheduleTable->rows) {
        reportError("hack_slo - called - but table ptr is null\n");
        return false;
    }
    for (int rowIndex = scheduleTable->rangeStart; rowIndex < scheduleTable->rangeEnd; rowIndex++) {
        ScheduleRow *row = &scheduleTable->rows[rowIndex];
        bool hasA = false;
        bool hasB = false;
        for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
            Instr *rowInstruction = row->entries[entryIndex].instruction;
            if (!rowInstruction)
                continue;
            for (int operandIndex = 0; operandIndex != rowInstruction->operandCount; operandIndex++) {
                Operand *operand = &rowInstruction->operands[operandIndex];
                if (isRegisterOperand(operand, 2)) {
                    if (operand->assignedRegister == registerEntry)
                        hasA = true;
                    if (operand->assignedRegister == alternateRegister)
                        hasB = true;
                }
            }
        }
        if (hasA && hasB)
            return true;
    }
    return false;
}

extern const char assignLocalExternalError[] = "AssignLocalRegisters - external assign hadn't happened!\n";
extern const char assignLocalModeError[] = "AssignLocalRegisters - can't do local assign on non-vsm table (%d)\n";
extern const char assignLocalCleanupError[] = "failed to clean up after local alloc\n";
extern const char assignLocalLocksError[] = "locks aren't valid for local register assign\n";
extern const char assignLocalPendingError[] = "nAssign - still things to do .. %d\n";
extern const char assignLocalBadTableError[] = "nAssignRegisters: istream had table, but was bad!\n";
extern const char buildTTabNoBitsError[] = "build_tTab - set no bits! for %s\n";
extern const char buildTTabOverlapError[] = "build_tTab - already set.. so some overlap %d?\n";
extern const char buildTTabLiveRangeError[] = "failed to get live range for an rLock\n";
extern const char buildTTabLockListError[] = "failed to get a lock list for %s\n";
extern const char buildTTabNullError[] = "build_tTab - passed a null pointer\n";
extern const char initAllocMaxDevError[] = "alloc - fatal - should catch earlier - m_nReg>=MAX_PER_DEV\n";
extern const char initAllocTooBigError[] = "table too big. coder can't be bothered to realloc\n";
extern const char initAllocDoneTableError[] = "regpool - assign registers -done table was bad\n";
extern const char initAllocNonZeroBaseError[] = "for now - can't base an alloc table other than at zero\n";
extern const char setTInATabOverlapError[] = "set_t_in_aTab - overlap detected at line %d, register %d our mask is %x vs %x!\n";
extern const char setTInATabAccessError[] = "couldnt fit at user has no access..id=%d\n";
extern const char setInATabBuildError[] = "failed to build ttab Forced build for mapped reg %s from %s\n";
extern const char setInATabCheckError[] = "failed to check_set t_in_aTab for '%s' at local ID=%d map\n";
extern const char setInATabLocalIdError[] = "failed to get local_id from a mapped register\n";
extern const char setInATabVirtualError[] = "RegPool::set_in_aTab,regref: alloc..had forced virtual register %s\n";

