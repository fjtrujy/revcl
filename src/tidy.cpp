#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"

static inline bool extendForEnd_inl(workTable *wt)
{
    do {
        int i = wt->unk_34++;
        wtRow *r = wt->func_080724f0(i);
        if (!r) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        r->func_080721f8();
    } while (wt->unk_34 - wt->unk_30 < 2);
    return true;
}

bool func_08062a88(Program *p)
{
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!s->workTablePtr) {
            reportError("work table not set in %s\n", s->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = s->workTablePtr;
        switch (wt->unk_09) {
        case 1:
            reportError("work table code mode is uta..%s\n", s->streamName());
            break;
        case 3:
        case 4:
            s->unk_21c = wt;
            break;
        default:
            reportError("bad code mode for Tidy\n");
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
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!s->unk_265)
            continue;
        linkNode *n = s->unk_134.unk_00;
        while (n) {
            linkNode *next = n->unk_08;
            if (!p->unLinkStreams(s, n->unk_0c, n->unk_14)) {
                reportError("failed disappear linkage\n");
                return false;
            }
            n = next;
        }
        Instr *b = s->getBranch();
        if (b)
            b->unk_1392 = 1;
        s->unk_2ce = 0;
        s->unk_2cd = 3;
        if (!s->workTablePtr) {
            reportError("work table not set in %s\n", s->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = s->workTablePtr;
        if (wt->unk_09 == 4)
            continue;
        if (wt->func_0807411c() < 0)
            continue;
        if (wt->getPhysicalLen() > 1 && wt->func_08073854())
            continue;
        if (!extendForEnd_inl(wt)) {
            reportError("failed to extend for e\n");
            return false;
        }
    }
    return true;
}

bool extendForEnd(workTable *wt)
{
    do {
        int i = wt->unk_34++;
        wtRow *r = wt->func_080724f0(i);
        if (!r) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        r->func_080721f8();
    } while (wt->unk_34 - wt->unk_30 < 2);
    return true;
}

extern const char fiddleBranchPosError[] = "the branch wasn't where I thought it was\n";
extern const char fiddleFailedPrevError[] = "after all that - failed to prev\n";
extern const char fiddleMoveBranchError[] = "after all that - failed to move branch\n";
extern const char fiddleGetPrevIeError[] = "reduceExt: failed to get prev ie\n";
extern const char fiddleGetBranchIeError[] = "reduceExt: failed to get branch ie\n";
extern const char fiddleGetLastIeError[] = "reduceExt: failed to get last ie\n";
extern const char fiddleSmallerError[] = "table already smaller than extminlength in fiddleBDL\n";
