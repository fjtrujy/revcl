#pragma implementation "fDep.h"
#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"
#include "fDep.h"

bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool func_08065fd4(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

Operand *getEA(Instr *i)
{
    iInfo *ii = i->unk_34;
    if (!ii) {
        func_0804ad38("no ii for getEA\n");
        func_0804adf0(-1);
    }
    if ((ii->unk_18 & 7) == 4) {
        if (ii->unk_1a >= i->unk_138b) {
            func_0804ad38("eaop is out of bounds for operands st=%d\n", ii->unk_19);
            i->leDump();
            func_0804adf0(-1);
        }
        Operand *op = &i->unk_38[ii->unk_1a];
        if (op->unk_09 != 1) {
            func_0804ad38("effective address specifier isn't an input\n");
            func_0804adf0(-1);
        }
        return op;
    }
    return 0;
}

bool sameEA(Instr *a, Instr *b)
{
    if (!a || !b || !a->unk_13ce || !b->unk_13ce || a->unk_30 != b->unk_30)
        return false;
    Operand *ea1 = getEA(a);
    Operand *ea2 = getEA(b);
    if (!ea1 || !ea2) {
        func_0804ad38("failed to get effective address operand\n");
        func_0804adf0(-1);
    }
    if (ea1->unk_27 != ea2->unk_27)
        return false;
    RegPoolEntry *r1 = ea1->unk_18;
    RegPoolEntry *r2 = ea2->unk_18;
    if (r1 && r1 == r1->unk_04->unk_10)
        r1 = 0;
    if (r2 && r2 == r2->unk_04->unk_10)
        r2 = 0;
    if (r1 != r2)
        return false;
    if (r1) {
        if (ea1->unk_07 != ea2->unk_07)
            return false;
        if (ea1->unk_07 != 1) {
            func_0804ad38("mod bits not 1 for an effective address specifier\n");
            func_0804adf0(-1);
        }
        if (!func_080585d4(ea1->unk_2c[0].unk_54.unk_00, ea2->unk_2c[0].unk_54.unk_00))
            return false;
    }
    if (func_0804e5c0(&ea1->unk_1c, &ea2->unk_1c))
        return true;
    return false;
}
