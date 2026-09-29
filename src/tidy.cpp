#pragma implementation "endPatch.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "endPatch.h"

bool func_08062a88(Program *p)
{
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!s->unk_218) {
            func_0804ad38("work table not set in %s\n", s->func_0806d674());
            func_0804adf0(-1);
        }
        workTable *wt = s->unk_218;
        switch (wt->unk_09) {
        case 1:
            func_0804ad38("work table code mode is uta..%s\n", s->func_0806d674());
            break;
        case 3:
        case 4:
            s->unk_21c = wt;
            break;
        default:
            func_0804ad38("bad code mode for Tidy\n");
            return false;
        }
        s->unk_265 = 1;
        if (s->unk_2cd != 1)
            s->unk_265 = 0;
        linkNode *n = s->unk_134.unk_00;
        if (!n)
            s->unk_265 = 0;
        while (n) {
            if (n->unk_0c->unk_2cd != 3) {
                s->unk_265 = 0;
                break;
            }
            n = n->unk_08;
        }
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!s->unk_265)
            continue;
        linkNode *n = s->unk_134.unk_00;
        while (n) {
            linkNode *next = n->unk_08;
            if (!p->unLinkStreams(s, n->unk_0c, n->unk_14)) {
                func_0804ad38("failed disappear linkage\n");
                return false;
            }
            n = next;
        }
        Instr *b = s->getBranch();
        if (b)
            b->unk_1392 = 1;
        s->unk_2ce = 0;
        s->unk_2cd = 3;
        if (!s->unk_218) {
            func_0804ad38("work table not set in %s\n", s->func_0806d674());
            func_0804adf0(-1);
        }
        workTable *wt = s->unk_218;
        if (wt->unk_09 == 4)
            continue;
        if (wt->func_0807411c() < 0)
            continue;
        if (wt->getPhysicalLen() > 1 && wt->func_08073854())
            continue;
        if (!extendForEnd(wt)) {
            func_0804ad38("failed to extend for e\n");
            return false;
        }
    }
    return true;
}
