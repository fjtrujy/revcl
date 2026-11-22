#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quietDiagnostics;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);
int branchLinkTypeForFlags(int flags);
bool fixupBtorBranch(Program *p, instStream *s, int *changed);
void dumpProgramStreamsAndRegisters(Program *p, int a);


bool resolveStreamBranches(Program *p)
{
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        switch (s->endType) {
        case 1: {
            unsigned char bt = s->branchType;
            int headFlags = bt;
            if (!(unsigned char)headFlags || (unsigned char)(headFlags & 4)) {
                if (s->nextStream) {
                    if (!p->linkStreams(s, s->nextStream, 0)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                    bt = s->branchType;
                } else if (!s->passThroughText) {
                    reportError("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                    return false;
                }
            }
            int flags = bt;
            if ((unsigned char)flags) {
                int btorMask = 1;
                int btor = (bt >> 1) & btorMask;
                if (!(unsigned char)btor) {
                    int type = branchLinkTypeForFlags(flags);
                    char *label = s->getBranchTargetImmediate();
                    if (!label) {
                        reportError("failed to get branch target immediate\n");
                        return false;
                    }
                    instStream *t = p->findStream(label);
                    if (!t) {
                        reportUserMessage("failed to find label '%s'\n", label);
                        return false;
                    }
                    if (!p->linkStreams(s, t, type)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                } else {
                    Instr *br = s->getBranch();
                    if (!br) {
                        reportError("failed to get a branch instruction for btor case\n");
                        return false;
                    }
                    if (br->branchLabelTags) {
                        int type = branchLinkTypeForFlags(s->branchType);
                        char name[2064];
                        for (char *q = br->branchLabelTags; *q; q++) {
                            char *r = q;
                            char *d = name;
                            while (*r != ':')
                                *d++ = *r++;
                            *d = 0;
                            q = r;
                            instStream *t = p->findStream(name);
                            if (!t) {
                                reportUserMessage("branch-to-register tag, failed to find label '%s'\n", name);
                                return false;
                            }
                            if (!p->linkStreams(s, t, type)) {
                                reportError("failed to link streams together\n");
                                return false;
                            }
                        }
                    }
                }
            }
            break;
        }
        case 4:
            if (!s->endDirectiveText) {
                reportError("no macro set in end_macro\n");
                return false;
            }
            /* fall through */
        case 3:
            if (s->branchType) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            break;
        case 2:
            if (s->branchType) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            if (!s->nextStream) {
                reportUserMessage("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                return false;
            }
            if (!p->linkStreams(s, s->nextStream, 0)) {
                reportError("failed to link streams together\n");
                return false;
            }
            break;
        default:
            reportError("bad end type in stream\n");
            return false;
        }
    }

    bool again;
    do {
        again = false;
        for (s = p->streamHead; s; s = s->nextStream) {
            int btorMask = 1;
            int btor = (s->branchType >> 1) & btorMask;
            if ((unsigned char)btor) {
                int changed = 0;
                if (!fixupBtorBranch(p, s, &changed)) {
                    reportError("failed to fixup a branch-to-register instruction\n");
                    return false;
                }
                if (changed)
                    again = true;
            }
        }
    } while (again);

    dumpProgramStreamsAndRegisters(p, 0);
    for (s = p->streamHead; s; s = s->nextStream) {
        Instr *br = s->getBranch();
        if (br && br->branchLabelTags) {
            delete[] br->branchLabelTags;
            br->branchLabelTags = 0;
        }
    }
    return true;
}

bool fixupBtorBranch(Program *p, instStream *s, int *changed)
{
    *changed = 0;
    if (((s->branchType >> 1) & 1) == 0) {
        reportError("balFix:: not reg-target\n");
        return false;
    }
    Instr *br = s->getBranch();
    if (!br) {
        reportError("balFix-no branch!\n");
        return false;
    }
    RegPoolEntry *reg = br->getBranchTargetRegister();
    if (!reg) {
        reportError("no target register input for btor\n");
        br->leDump();
        return false;
    }
    br->dumpOperandDiagnostics(false);
    instStream *t;
    for (t = p->streamHead; t; t = t->nextStream)
        t->visitState = 0;
    s->visitState = 1;
    bool first = true;
    bool again;
    do {
        again = false;
        for (t = p->streamHead; t; t = t->nextStream) {
            if (t->visitState != 1)
                continue;
            workTable *wt = t->getTable();
            Instr *in;
            if (first) {
                in = br->linearLink.previousInstruction;
                t->visitState = 0;
                first = false;
            } else {
                in = wt->linearInstructions.tail;
                t->visitState = 2;
            }
            for (; in; in = in->linearLink.previousInstruction) {
                if (in->outputMaskForRegister(reg, 0xff)) {
                    in->dumpOperandDiagnostics(false);
                    break;
                }
            }
            if (in) {
                if ((in->branchFlags >> 3) & 1) {
                    if (!t->nextStream) {
                        reportUserMessage("no physical successor to a branch+link\n");
                        return false;
                    }
                    if (!p->areLinkedBy(s, t->nextStream, 2)) {
                        if (!p->linkStreams(s, t->nextStream, 2)) {
                            reportError("failed to fixup a branch to register\n");
                            return false;
                        }
                        (*changed)++;
                        again = true;
                    }
                } else if (!br->branchLabelTags) {
                    reportError("VCL has found a branch to register at :\n");
                    br->dumpOperandDiagnostics(false);
                    reportError("that's apparently set by:\n");
                    in->dumpOperandDiagnostics(false);
                    reportError("unfortunately vcl can't figure out where the branch should go..\n");
                    reportError("try adding a tag list to the branch\n");
                    return false;
                }
            } else {
                for (linkNode *n = t->inputLinks.head; n; n = n->next) {
                    instStream *o = n->otherStream;
                    if (!o) {
                        reportError("null outist\n");
                        return false;
                    }
                    if (!o->visitState) {
                        o->visitState = 1;
                        again = true;
                    }
                }
            }
        }
    } while (again);
    return true;
}
