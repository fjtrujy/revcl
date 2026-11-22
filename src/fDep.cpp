#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"

bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool func_08065fd4(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

Operand *getEA(Instr *i)
{
    iInfo *ii = i->opcodeInfo;
    if (!ii) {
        reportError("no ii for getEA\n");
        exitAfterCleanup(-1);
    }
    if ((ii->unk_18 & 7) == 4) {
        if (ii->unk_1a >= i->operandCount) {
            reportError("eaop is out of bounds for operands st=%d\n", ii->unk_19);
            i->leDump();
            exitAfterCleanup(-1);
        }
        Operand *op = &i->operands[ii->unk_1a];
        if (op->unk_09 != 1) {
            reportError("effective address specifier isn't an input\n");
            exitAfterCleanup(-1);
        }
        return op;
    }
    return 0;
}

static inline bool sameValue(Operand *a, Operand *b)
{
    return func_0804e5c0(&a->unk_1c, &b->unk_1c);
}

static inline bool sameOwner(Instr *a, Instr *b)
{
    if (a->unk_30 == b->unk_30)
        return true;
    return false;
}

bool sameEA(Instr *a, Instr *b)
{
    if (!a || !b || !a->unk_13ce || !b->unk_13ce || !sameOwner(a, b))
        return false;
    Operand *ea1 = getEA(a);
    Operand *ea2 = getEA(b);
    if (!ea1 || !ea2) {
        reportError("failed to get effective address operand\n");
        exitAfterCleanup(-1);
    }
    if (ea1->unk_27 != ea2->unk_27)
        return false;
    RegPoolEntry *r1 = ea1->unk_18;
    RegPoolEntry *r2 = ea2->unk_18;
    if (r1 && r1 == r1->ownerPool->zeroRegister)
        r1 = 0;
    if (r2 && r2 == r2->ownerPool->zeroRegister)
        r2 = 0;
    if (r1 != r2)
        return false;
    if (r1) {
        if (ea1->unk_07 != ea2->unk_07)
            return false;
        if (ea1->unk_07 != 1) {
            reportError("mod bits not 1 for an effective address specifier\n");
            exitAfterCleanup(-1);
        }
        if (!func_080585d4(ea1->unk_2c[0].unk_54.head, ea2->unk_2c[0].unk_54.head))
            return false;
    }
    return sameValue(ea1, ea2);
}
