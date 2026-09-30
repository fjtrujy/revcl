#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

bool checkOpt1(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("null work table\n");
            return false;
        }
        if (wt != &s->unk_148) {
            func_0804ad38("work table isn't input table\n");
            return false;
        }
        s->getAllocMode();
    }
    return true;
}

bool checkOpt2(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("null work table\n");
            return false;
        }
        if (s->func_0806d908() == 4) {
            if (s->getTable() != &s->unk_148) {
                func_0804ad38("checkopt2 - synth table wasn't input\n");
                return false;
            }
        } else {
            if (s->getTable() != s->unk_214) {
                func_0804ad38("work table isn't opt table\n");
                return false;
            }
            if (!wt->unk_0b) {
                func_0804ad38("locks aren't valid for opt table\n");
                return false;
            }
        }
        if (s->getAllocMode() != 3) {
            func_0804ad38("table %s didn't have external alloc\n", s->func_0806d674());
            return false;
        }
    }
    return true;
}
