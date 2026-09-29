#include "Program.h"
#include "instStream.h"
#include "workTable.h"

int func_0804ad38(const char *fmt, ...);
bool func_08061c34(instStream *n);
bool func_080615b0(Program *p, instStream *n);
bool func_080614bc(Program *p);
int func_08061cb0(Program *p, instStream *n);

bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b)
{
    if (!t) {
        func_0804ad38("int_buildTpairs - null arg\n");
        return false;
    }
    p->unk_d4 = t;
    p->unk_dc = 0;
    p->unk_e0 = 0;
    p->unk_e4 = 0;
    p->unk_b5 = 0;
    p->unk_b4 = 0;
    for (instStream *i = p->unk_a0; i; i = i->unk_108) {
        if (!func_08061c34(i)) {
            func_0804ad38("failed to do a local init for the back trace\n");
            return false;
        }
    }
    if (!func_080615b0(p, t)) {
        func_0804ad38("failed on the initial extend\n");
        return false;
    }
    for (instStream *i = p->unk_e4; i; i = i->unk_25c) {
        i->unk_244 = 0;
        i->unk_24c = 0;
        i->unk_240 = 0;
        i->unk_248 = 0;
    }
    if (!func_080614bc(p)) {
        func_0804ad38("failed to do internal trace for  a t pair\n");
        return false;
    }
    regUse *tr = t->unk_234;
    if (!tr) {
        func_0804ad38("there's  no regref set in t target\n");
        return false;
    }
    if (tr->unk_08 != reg) {
        func_0804ad38("there's a mismatch between reg and regref\n");
        return false;
    }
    for (instStream *s = p->unk_dc; s; s = s->unk_250) {
        regUse *sr = s->unk_234;
        if (!sr) {
            func_0804ad38("there's no regref in t src\n");
            return false;
        }
        if (sr->unk_08 != reg) {
            func_0804ad38("mismatch between regref reg and rg\n");
            return false;
        }
        int dst_key = t->unk_23c;
        int src_key = func_08061cb0(p, s);
        if (dst_key < 0) {
            func_0804ad38("dst_key is bad in tpair\n");
            return false;
        }
        if (src_key < 0) {
            func_0804ad38("src_key is bad in tpair\n");
            return false;
        }
        if (!tr->unk_20[part].unk_10.addPair(s, b, src_key, dst_key)) {
            func_0804ad38("failed to add tpair for tcheck\n");
            return false;
        }
    }
    return true;
}

const char *regTypeName(int type)
{
    const char *s = "UNKNOWN_TYPE";
    switch (type) {
    case 8:  s = "VPU_I"; break;
    case 1:  s = "CPU_GPR"; break;
    case 2:  s = "FPR"; break;
    case 6:  s = "CPU_HW"; break;
    case 9:  s = "VPU_VF"; break;
    case 10: s = "VPU_HW"; break;
    }
    return s;
}
