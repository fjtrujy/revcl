#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

static inline void copyShort(short &to, const short &from)
{
    to = from;
}

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
extern int g_080b33f0;

workTable *makeForceTable(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        const char *modeName = codeModeName(mode);
        func_0804ad38("it_old is not raw mode - %s\n", modeName);
        return 0;
    }
    workTable *nt = prepUta(p, s, 0, 0);
    if (!nt) {
        func_0804ad38("failed to make a lock table for makeForceTable\n");
        return 0;
    }
    mode = nt->unk_09;
    if ((unsigned char)mode != 2) {
        const char *modeName = codeModeName(mode);
        func_0804ad38("it_new is not raw mode .. %s\n", modeName);
        return 0;
    }
    int start = wt->func_0807411c();
    int end = wt->func_080741c0();
    nt->setRange(start, end);
    for (Instr *in = nt->unk_94.unk_00; in; in = nt->unk_94.unk_00) {
        Instr *clone = in->unk_08;
        cls08058718 pos;
        cls08058718 *r = &clone->unk_13e8;
        pos.unk_00 = r->unk_00;
        pos.unk_04 = r->unk_04;
        pos.unk_03 = r->unk_03;
        pos.unk_02 = r->unk_02;
        if (!nt->forceInsert(in, &pos)) {
            func_0804ad38("failed to force an insert into table\n");
            delete nt;
            return 0;
        }
    }
    nt->changeMode(3);
    nt->func_08073774(wt->func_08073754());
    return nt;
}
