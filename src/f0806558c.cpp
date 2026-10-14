#include "fDep_shared.h"

bool func_0806558c(Instr *i, int *row)
{
    *row = 0;
    for (ol_s *o = i->unk_13a0.unk_00; o; o = o->unk_00) {
        Instr *in = o->unk_04;
        if (!in) {
            func_0804ad38("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->unk_13e8.unk_00;
        if (r < 0) {
            *row = -1;
            return true;
        }
        if (r + 1 > *row)
            *row = r + 1;
    }
    return true;
}

