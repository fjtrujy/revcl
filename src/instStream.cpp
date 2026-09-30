#include "instStream.h"

instStream::instStream()
{
    unk_290.clear();
    unk_2d8 = 0;
    unk_144 = 0;
    unk_140 = 0;
    unk_2b8 = 0;
    unk_2ba = 0;
    unk_2e0 = -1;
    unk_2e4 = -1;
    unk_2bc = 0x4000;
    unk_2dc = 0;
    unk_268 = 0;
    unk_26c = 0;
    unk_2c4 = 0;
    unk_270 = 0;
    unk_274 = 0;
    unk_2c8 = -1;
    unk_238 = 0;
    unk_2d0 = 0;
    unk_108 = 0;
    unk_10c = 0;
    unk_2c0 = 0;
    unk_2c2 = 0;
    unk_2c6 = 0;
    unk_104 = 0;
    unk_218 = 0;
    unk_220 = 0;
    unk_214 = 0;
    unk_21c = 0;
    unk_124 = 0;
    unk_2d6 = 0;
    unk_2d7 = 0;
    unk_2d4 = 0;
    unk_2d5 = 0;
    unk_260 = 0;
    unk_261 = 0;
    unk_262 = 0;
    unk_2b6 = -1;
    unk_2b4 = -1;
    unk_2ce = 0;
    unk_2cd = 1;
    unk_2cc = 1;
    unk_00 = 0;
}

instStream::~instStream()
{
    reset();
}

bool instStream::reset()
{
    unk_2b8 = 0;
    unk_2ba = 0;
    unk_110.func_0804b1b0();
    unk_2c0 = -1;
    unk_104 = 0;
    unk_2ce = 0;
    if (unk_238)
        delete[] unk_238;
    unk_238 = 0;
    if (unk_2d0)
        delete[] unk_2d0;
    unk_2d0 = 0;
    unk_128.removeAll();
    unk_134.removeAll();
    if (unk_124)
        delete[] unk_124;
    unk_124 = 0;
    return true;
}

void instStream::dump(bool)
{
    unk_110.func_0804b534();
    unk_128.dump();
    canLoop();
    workTable *wt = unk_218;
    if (!wt) {
        func_0804ac7c("iStream:: dump - null work table - defaulting to inputTable\n");
        wt = &unk_148;
    }
    wt->func_08072c70();
    unk_134.dump();
    int dep = func_0805f284();
    int log = func_0805f29c();
    // length print compiled out in this build
    if (dep > 0 || log > 0) {
    }
}

void instStream::zeroStats()
{
    if (!unk_218) {
        func_0804ad38("no work table set for zeroStats!\n");
        func_0804adf0(-1);
    }
    for (Instr *i = unk_218->unk_88.unk_00; i; i = i->unk_1c.unk_00)
        i->unk_13f0.func_0805e340();
    unk_278.func_0805e340();
}

bool instStream::iOwn(Instr *i)
{
    if (!i) {
        func_0804ac7c("iStream::iOwn - passed a null pointer\n");
        return false;
    }
    instStream *o = i->func_08054168();
    if (!o) {
        func_0804ac7c("iStream::iOwn - no owner for instruction\n");
        return false;
    }
    return o == this;
}

bool instStream::patchLatencies()
{
    workTable *wt = unk_218;
    int n = 0;
    if (!wt) {
        func_0804ad38("iStream::PatchLatencies - no work table\n");
        return false;
    }
    for (Instr *i = wt->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
        i->unk_13e4 = n;
        if (!i->func_08052f84()) {
            func_0804ad38("failed to patch latencies for instruction\n");
            return false;
        }
        n++;
    }
    return true;
}

const char *istEndTypeName(int t)
{
    const char *s = "ISTET_BAD";
    switch (t) {
    case 1:
        s = "ISTET_NULL";
        break;
    case 2:
        s = "ISTET_END_CONT";
        break;
    case 3:
        s = "ISTET_END_END";
        break;
    case 4:
        s = "ISTET_END_MACRO";
        break;
    }
    return s;
}
