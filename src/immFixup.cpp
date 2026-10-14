#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

bool aaFixup(instStream *s);
bool func_08069e7c(Instr *in);
int maddImmediate(Operand *op, mValue *v, int n);

static inline void initImm(Instr *in)
{
    in->unk_13d4 = 0;
    in->unk_13d5 = 0;
    in->unk_13d6 = -1;
    in->unk_13d8.clear();
    in->unk_13d2 = -1;
}

static inline bool initStreamImm(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("Init_immFixup - failed to get work table\n");
        return false;
    }
    s->unk_2c2 = 0;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        initImm(in);
    return true;
}

static inline void fixInst(Instr *in)
{
    if (!func_08069e7c(in)) {
        func_0804ad38("do_immFixup - failed for an inst\n");
        func_0804adf0(-1);
    }
}

static inline bool do_immFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt)
        return true;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        fixInst(in);
    return true;
}

static inline bool dumpFailed(instStream *s)
{
    s->dump(false);
    return false;
}

bool func_08069324(Program *p, bool init)
{
    instStream *s;
    if (init) {
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!initStreamImm(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!aaFixup(s))
                return dumpFailed(s);
    } else {
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!do_immFixup(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!initStreamImm(s))
                return false;
    }
    return true;
}

