#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

int g_080b33f0 = 1;

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart)
{
    workTable *wt = s->getTable();
    if (loopSize) {
        if (!s->canLoop()) {
            func_0804ad38("prepLockTable: can't loop, but loop size requested\n");
            return 0;
        }
        if (wt->unk_09 == 3) {
            func_0804ad38("prepLockTable: loopSize requested, but mode is RAW\n");
            return 0;
        }
    }
    if (!optReset(p)) {
        func_0804ad38("failed to optReset cpu\n");
        return 0;
    }
    if (!wt->clearClones()) {
        func_0804ad38("failed to flush workClones table for %s\n", s->func_0806d674());
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        func_0804ad38("iStream::prepUta - failed alloc table\n");
        func_0804adf0(-1);
    }
    if (!nt->init(2, s->unk_2bc)) {
        func_0804ad38("failed to allocate large table\n");
        delete nt;
        return 0;
    }
    nt->unk_10 = s;
    if (loopSize) {
        nt->unk_7a = s->unk_274;
        nt->setRange(loopSize);
    } else
        nt->setRange();
    setRangeFrom(nt);
    g_080de284 = smart;
    nt->unk_5c.func_0805e37c(&s->unk_278);
    nt->unk_74 = wt->unk_74;
    nt->unk_76 = wt->unk_76;
    nt->unk_78 = wt->unk_78;
    int mode = (unsigned char)wt->unk_08;
    nt->unk_08 = mode;
    nt->unk_40 = 0;
    nt->unk_44 = wt->unk_44;
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        if (!nt->qRegRef(ru)) {
            func_0804ad38("failed to add a cloned regref in %s\n", s->func_0806d674());
            wt->dump();
            delete nt;
            return 0;
        }
    }
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (in->func_08054168() != s) {
            func_0804ad38("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          in->func_08054168(), p);
            in->leDump();
            func_0804adf0(-1);
        }
        in->unk_141a = 0;
        if (in->func_080541fc() && !in->func_08054138() && !nt->qInstruction(in)) {
            func_0804ad38("iStream: failed to queue instruction into table for opt\n");
            func_0804adf0(-1);
        }
    }
    if (!fixupCloneFdeps(nt)) {
        func_0804ad38("failed to fix fdeps to new table\n");
        return 0;
    }
    return nt;
}

workTable *makeForceTable(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        func_0804ad38("it_old is not raw mode - %s\n", codeModeName(mode));
        return 0;
    }
    workTable *nt = prepUta(p, s, 0, 0);
    if (!nt) {
        func_0804ad38("failed to make a lock table for makeForceTable\n");
        return 0;
    }
    mode = nt->unk_09;
    if ((unsigned char)mode != 2) {
        func_0804ad38("it_new is not raw mode .. %s\n", codeModeName(mode));
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
