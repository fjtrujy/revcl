// Global prune: remove streams unreachable from the top and redundant branches.
#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

extern int errorOccurred;
extern bool quietDiagnostics;

int branchLinkTypeForFlags(int flags);

bool globalPrune(Program *program)
{
    instStream *s;
    for (s = program->streamHead; s; s = s->nextStream) {
        if (s->startType == 2) {
            s->reachableFromEntry = 1;
            s->visitState = 1;
        } else {
            s->reachableFromEntry = 0;
            s->visitState = 0;
        }
        if (s->passThroughText) {
            reportInfo("pass thru  - setting reached top for %s\n", s->streamName());
            s->reachableFromEntry = 1;
        }
    }

    bool changed;
    do {
        changed = false;
        for (s = program->streamHead; s; s = s->nextStream) {
            if (s->visitState == 1) {
                s->visitState = 2;
                for (linkNode *link = s->outputLinks.head; link; link = link->next) {
                    instStream *o = link->otherStream;
                    if (!o) {
                        reportError("null outist\n");
                        return false;
                    }
                    o->reachableFromEntry = 1;
                    if (o->visitState == 0) {
                        o->visitState = 1;
                        changed = true;
                    }
                }
            }
        }
    } while (changed);

    instStream *next;
    for (s = program->streamHead; s; s = next) {
        next = s->nextStream;
        if (s->reachableFromEntry)
            continue;
        reportInfo("prune inaccessible stream %s\n", s->streamName());
        workTable *t = s->getTable();
        for (Instr *in = t->linearInstructions.head; in; in = in->linearLink.nextInstruction)
            if (!in->isNop()) {
                reportInfo("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                in->leDump();
                errorOccurred = saved;
            }
        linkNode *l;
        while ((l = s->inputLinks.head) != 0) {
            if (!program->unlinkStreams(l->otherStream, s, l->linkType)) {
                reportError("unlink failed for input\n");
                return false;
            }
        }
        while ((l = s->outputLinks.head) != 0) {
            if (!program->unlinkStreams(s, l->otherStream, l->linkType)) {
                reportError("unlink failed for output\n");
                return false;
            }
        }
        if (!program->removeStream(s)) {
            reportError("failed to unlink an ist\n");
            return false;
        }
        delete s;
    }

    instStream *follow;
    for (s = program->streamHead; s; s = follow) {
        follow = s->nextStream;
        instStream *next = follow;
        if (next && s->branchType && s->getRawSchedulingMode() != 4) {
            bool keepLink = false;
            linkNode *l;
            for (l = s->outputLinks.head; l; l = l->next) {
                if (l->otherStream != next)
                    break;
                if (!l->linkType)
                    keepLink = true;
            }
            if (!l) {
                reportInfo("remove branch..\n");
                Instr *br = s->getBranch();
                if (!br) {
                    reportError("which wasn't there?\n");
                    return false;
                }
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                br->dumpOperandDiagnostics(false);
                errorOccurred = saved;
                int type = branchLinkTypeForFlags(s->branchType);
                if (!program->unlinkStreams(s, next, type)) {
                    reportError("failed to unlink the streams through the branch\n");
                    return false;
                }
                if (!s->removeBranch()) {
                    reportError("failed to remove branch instruction\n");
                    return false;
                }
                if (!keepLink && !program->linkStreams(s, next, 0)) {
                    reportError("failed to relink two streams following removing an absolute branch\n");
                    return false;
                }
            }
        }
    }
    return true;
}
