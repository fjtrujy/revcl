#include "os.h"
#include "Instr.h"
#include "ol_s.h"

bool copyCloneFdeps(Instr *, ol_s *o, olList *dst)
{
    for (; o; o = o->unk_00) {
        Instr *in = o->unk_04;
        if (!in) {
            func_0804ad38("qInstruction.. fDepList in_f not set in ol\n");
            return false;
        }
        Instr *clone = in->unk_08;
        if (!clone) {
            func_0804ad38("qInstruction - fdep list - clone image not set during rebuild\n");
            return false;
        }
        if (!dst->func_0805826c(clone)) {
            func_0804ad38("qInstruction - failed to add an OL to the fdep list from clone\n");
            return false;
        }
    }
    return true;
}

bool func_08065fd4(Instr *in, Instr *other)
{
    in->func_08054008(false);
    other->func_08054008(false);
    for (int k = 0; k != other->unk_138b; k++) {
        Operand *op = &other->unk_38[k];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            for (int p = 0; p != r->unk_28; p++)
                if ((op->unk_07 >> p) & 1)
                    for (ol_s *l = op->unk_2c[p].unk_54.unk_00; l; l = l->unk_00) {
                        Instr *x = l->unk_04;
                        if (x && x != in && x->unk_13e4 < in->unk_13e4) {
                            in->unk_1410 = 1;
                            if (!in->unk_13a0.func_0805826c(x)) {
                                func_0804ad38("failed to add\n");
                                func_0804adf0(-1);
                            }
                            x->func_08054008(false);
                        }
                    }
        }
    }
    return true;
}
