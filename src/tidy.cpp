#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"

static inline bool extendForEnd_inl(workTable *wt)
{
    do {
        int i = wt->rangeEnd++;
        wtRow *r = wt->getRowByIndex(i);
        if (!r) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        r->clearEntries();
    } while (wt->rangeEnd - wt->rangeStart < 2);
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
        switch (wt->codeModeValue) {
        case 1:
            reportError("work table code mode is uta..%s\n", s->streamName());
            break;
        case 3:
        case 4:
            s->emissionTable = wt;
            break;
        default:
            reportError("bad code mode for Tidy\n");
            return false;
        }
        s->mergeIntoSuccessor = 1;
        if (s->endType != 1)
            s->mergeIntoSuccessor = 0;
        linkNode *n = s->outputLinks.head;
        if (!n)
            s->mergeIntoSuccessor = 0;
        while (n) {
            if (n->otherStream->endType != 3) {
                s->mergeIntoSuccessor = 0;
                break;
            }
            n = n->next;
        }
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!s->mergeIntoSuccessor)
            continue;
        linkNode *n = s->outputLinks.head;
        while (n) {
            linkNode *next = n->next;
            if (!p->unLinkStreams(s, n->otherStream, n->linkType)) {
                reportError("failed disappear linkage\n");
                return false;
            }
            n = next;
        }
        Instr *b = s->getBranch();
        if (b)
            b->tildePrefix = 1;
        s->branchType = 0;
        s->endType = 3;
        if (!s->workTablePtr) {
            reportError("work table not set in %s\n", s->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = s->workTablePtr;
        if (wt->codeModeValue == 4)
            continue;
        if (wt->getRangeStart() < 0)
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
        int i = wt->rangeEnd++;
        wtRow *r = wt->getRowByIndex(i);
        if (!r) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        r->clearEntries();
    } while (wt->rangeEnd - wt->rangeStart < 2);
    return true;
}

extern const char fiddleBranchPosError[] = "the branch wasn't where I thought it was\n";
extern const char fiddleFailedPrevError[] = "after all that - failed to prev\n";
extern const char fiddleMoveBranchError[] = "after all that - failed to move branch\n";
extern const char fiddleGetPrevIeError[] = "reduceExt: failed to get prev ie\n";
extern const char fiddleGetBranchIeError[] = "reduceExt: failed to get branch ie\n";
extern const char fiddleGetLastIeError[] = "reduceExt: failed to get last ie\n";
extern const char fiddleSmallerError[] = "table already smaller than extminlength in fiddleBDL\n";
