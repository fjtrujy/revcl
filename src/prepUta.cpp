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

int g_080b33f0 = 1;

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart)
{
    workTable *wt = s->getTable();
    if (loopSize) {
        if (!s->canLoop()) {
            reportError("prepLockTable: can't loop, but loop size requested\n");
            return 0;
        }
        if (wt->unk_09 == 3) {
            reportError("prepLockTable: loopSize requested, but mode is RAW\n");
            return 0;
        }
    }
    if (!optReset(p)) {
        reportError("failed to optReset cpu\n");
        return 0;
    }
    if (!wt->clearClones()) {
        reportError("failed to flush workClones table for %s\n", s->streamName());
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        reportError("iStream::prepUta - failed alloc table\n");
        exitAfterCleanup(-1);
    }
    if (!nt->init(2, s->unk_2bc)) {
        reportError("failed to allocate large table\n");
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
    copyShort(nt->unk_74, wt->unk_74);
    copyShort(nt->unk_76, wt->unk_76);
    copyShort(nt->unk_78, wt->unk_78);
    int mode = (unsigned char)wt->unk_08;
    nt->unk_08 = mode;
    nt->unk_40 = 0;
    nt->unk_44 = wt->unk_44;
    regUseList &regs = wt->unk_14;
    for (regUse *ru = regs.head; ru; ru = ru->unk_04) {
        if (!nt->qRegRef(ru)) {
            reportError("failed to add a cloned regref in %s\n", s->streamName());
            wt->dump();
            delete nt;
            return 0;
        }
    }
    for (Instr *in = wt->unk_88.head; in; in = in->unk_1c.unk_00) {
        if (in->func_08054168() != s) {
            reportError("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          in->func_08054168(), p);
            in->leDump();
            exitAfterCleanup(-1);
        }
        in->unk_141a = 0;
        if (in->func_080541fc() && !in->func_08054138() && !nt->qInstruction(in)) {
            reportError("iStream: failed to queue instruction into table for opt\n");
            exitAfterCleanup(-1);
        }
    }
    if (!fixupCloneFdeps(nt)) {
        reportError("failed to fix fdeps to new table\n");
        return 0;
    }
    return nt;
}

