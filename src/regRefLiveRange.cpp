#include "os.h"
#include "genRegPool_shared.h"

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *GetRP(unsigned t);
bool resetProgramAllocationsForFirstPass(Program *p);
bool resetProgramAllocationsForSecondPass(Program *p);
bool setRange(int start, int end, int lsz);
int countSetBits(unsigned int);
int setRangeFrom(workTable *t);


static inline bool validPipe(int p)
{
    return p == 0 || p == 1;
}

bool regRefLiveRange(regUse *ru, int part, int *s, int *e, bool *always)
{
    *s = -4;
    *e = -3;
    *always = false;
    regUseSlot *sl = &ru->slots[part];
    if ((char)bitSet(ru->inputMask, part)) {
        if (sl->timingValues[0] < 0 || !validPipe(sl->timingValues[1]) ||
            sl->timingValues[3] < 0 || !validPipe(sl->timingValues[4])) {
            reportError("firstInput<0.. but iBit set\n");
            exitAfterCleanup(-1);
        }
    }
    if ((char)bitSet(ru->outputMask, part)) {
        if (sl->timingValues[5] < 0 || !validPipe(sl->timingValues[6]) ||
            sl->timingValues[8] < 0 || !validPipe(sl->timingValues[9])) {
            reportError("firstInput<0.. but iBit set\n");
            exitAfterCleanup(-1);
        }
    }
    if ((char)bitSet(ru->throughputMask & ~(ru->outputMask | ru->inputMask), part)) {
        *s = -1;
        *e = 30000;
        *always = true;
        return true;
    }
    if ((char)bitSet(ru->externalInputMask, part)) {
        *s = -1;
    } else {
        if ((char)bitSet(ru->inputMask, part) && sl->timingValues[0] <= sl->timingValues[5]) {
            reportError("first input before first output.. but not xi %d,%d!\n",
                          sl->timingValues[0], sl->timingValues[5]);
            exitAfterCleanup(-1);
        }
        if ((char)bitSet(ru->outputMask, part))
            *s = sl->timingValues[5];
    }
    if ((char)bitSet(ru->throughputMask | ru->externalOutputMask, part)) {
        *e = 30000;
    } else {
        if ((char)bitSet(ru->outputMask, part))
            *e = sl->timingValues[8] + 1;
        if ((char)bitSet(ru->inputMask, part) && sl->timingValues[3] > *e)
            *e = sl->timingValues[3];
    }
    int row = *s;
    if (*e <= row) {
        row = *e;
        reportError("liverange for regref has e<=s .. %d,%d?\n", *s, row);
        return false;
    }
    return true;
}

static inline bool isInternalLock(OperandLock *lk)
{
    return lk->ownerType == 2;
}

static inline bool isInternalIn(OperandLock *lk)
{
    return lk->unk_19 == 2;
}

bool lockLiveRange(RegisterPartLockList *ll, OperandLock *out, int *s, int *e)
{
    *s = -1;
    *e = -1;
    if (!isInternalLock(out)) {
        Instr *ins = out->ownerInstr();
        if (!ins) {
            reportError("failed to get instruction for non -internal input\n");
            return false;
        }
        InstructionPosition *rl = &ins->position;
        int line = rl->getRow();
        int want = out->unk_32;
        if ((short)line != (short)want) {
            reportError("rLine mismatch\n");
            return false;
        }
        if (!validPipe(rl->getPipe())) {
            reportError("bad pipe for instruction\n");
            return false;
        }
        *s = line;
    } else {
        *s = -1;
    }
    if (out->unk_16) {
        if (g_080b336c && *s >= 0)
            *e = *s + g_080b336c - *s % g_080b336c;
        else
            *e = 30000;
    } else {
        *e = -1;
    }
    for (OperandLock *c = out->parentOrChildren; c; c = c->nextSibling) {
        if (isInternalIn(c))
            continue;
        Instr *ci = c->ownerInstr();
        if (!ci) {
            reportError("no inst for rl\n");
            return false;
        }
        InstructionPosition *rl = &ci->position;
        int line = rl->getRow();
        int want = c->unk_32;
        if ((short)line != (short)want) {
            reportError("rline mismatch for input locks\n");
            return false;
        }
        if (!validPipe(rl->getPipe())) {
            reportError("bad pipe for instruction\n");
            return false;
        }
        if ((short)line <= *s) {
            reportError("el<=s, but can't age\n");
            return false;
        }
        if ((short)line > *e)
            *e = (short)line;
    }
    if (*e < 0)
        *e = *s + 1;
    return true;
}

bool hack_slo(workTable *wt, RegPoolEntry *a, RegPoolEntry *b)
{
    if (wt->codeModeValue != 3) {
        reportError("hack_slo - called - but table not raw\n");
        return false;
    }
    if (!wt->rows) {
        reportError("hack_slo - called - but table ptr is null\n");
        return false;
    }
    for (int r = wt->rangeStart; r < wt->rangeEnd; r++) {
        wtRow *row = &wt->rows[r];
        bool hasA = false;
        bool hasB = false;
        for (int j = 0; j != 2; j++) {
            Instr *in = row->entries[j].unk_00;
            if (!in)
                continue;
            for (int i = 0; i != in->operandCount; i++) {
                Operand *op = &in->operands[i];
                if (isRegisterOperand(op, 2)) {
                    if (op->assignedRegister == a)
                        hasA = true;
                    if (op->assignedRegister == b)
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

