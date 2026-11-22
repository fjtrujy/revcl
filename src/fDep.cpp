#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"

bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool addAdditionalClonePredecessors(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

Operand *getEA(Instr *i)
{
    iInfo *ii = i->opcodeInfo;
    if (!ii) {
        reportError("no ii for getEA\n");
        exitAfterCleanup(-1);
    }
    if ((ii->kindAndFlags & 7) == 4) {
        if (ii->branchTargetOperandSelector >= i->operandCount) {
            reportError("eaop is out of bounds for operands st=%d\n", ii->kindData);
            i->leDump();
            exitAfterCleanup(-1);
        }
        Operand *op = &i->operands[ii->branchTargetOperandSelector];
        if (op->direction != 1) {
            reportError("effective address specifier isn't an input\n");
            exitAfterCleanup(-1);
        }
        return op;
    }
    return 0;
}

static inline bool sameValue(Operand *a, Operand *b)
{
    return sameImmediateValue(&a->immediateValue, &b->immediateValue);
}

static inline bool sameOwner(Instr *a, Instr *b)
{
    if (a->ownerTable == b->ownerTable)
        return true;
    return false;
}

bool sameEA(Instr *a, Instr *b)
{
    if (!a || !b || !a->ramAccessType || !b->ramAccessType || !sameOwner(a, b))
        return false;
    Operand *ea1 = getEA(a);
    Operand *ea2 = getEA(b);
    if (!ea1 || !ea2) {
        reportError("failed to get effective address operand\n");
        exitAfterCleanup(-1);
    }
    if (ea1->externalPartMask != ea2->externalPartMask)
        return false;
    RegPoolEntry *r1 = ea1->assignedRegister;
    RegPoolEntry *r2 = ea2->assignedRegister;
    if (r1 && r1 == r1->ownerPool->zeroRegister)
        r1 = 0;
    if (r2 && r2 == r2->ownerPool->zeroRegister)
        r2 = 0;
    if (r1 != r2)
        return false;
    if (r1) {
        if (ea1->componentMask != ea2->componentMask)
            return false;
        if (ea1->componentMask != 1) {
            reportError("mod bits not 1 for an effective address specifier\n");
            exitAfterCleanup(-1);
        }
        if (!sameOperandLocation(ea1->parts[0].operandLocations.head, ea2->parts[0].operandLocations.head))
            return false;
    }
    return sameValue(ea1, ea2);
}
