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


bool resolveStreamBranches(Program *program)
{
    instStream *currentStream;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        switch (currentStream->endType) {
        case 1: {
            unsigned char branchType = currentStream->branchType;
            int headFlags = branchType;
            if (!(unsigned char)headFlags || (unsigned char)(headFlags & 4)) {
                if (currentStream->nextStream) {
                    if (!program->linkStreams(currentStream, currentStream->nextStream, 0)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                    branchType = currentStream->branchType;
                } else if (!currentStream->passThroughText) {
                    reportError("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                    return false;
                }
            }
            int flags = branchType;
            if ((unsigned char)flags) {
                int btorMask = 1;
                int btor = (branchType >> 1) & btorMask;
                if (!(unsigned char)btor) {
                    int type = branchLinkTypeForFlags(flags);
                    char *label = currentStream->getBranchTargetImmediate();
                    if (!label) {
                        reportError("failed to get branch target immediate\n");
                        return false;
                    }
                    instStream *targetStream = program->findStream(label);
                    if (!targetStream) {
                        reportUserMessage("failed to find label '%s'\n", label);
                        return false;
                    }
                    if (!program->linkStreams(currentStream, targetStream, type)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                } else {
                    Instr *branchInstruction = currentStream->getBranch();
                    if (!branchInstruction) {
                        reportError("failed to get a branch instruction for btor case\n");
                        return false;
                    }
                    if (branchInstruction->branchLabelTags) {
                        int type = branchLinkTypeForFlags(currentStream->branchType);
                        char name[2064];
                        for (char *labelTagCursor = branchInstruction->branchLabelTags; *labelTagCursor; labelTagCursor++) {
                            char *tagTextCursor = labelTagCursor;
                            char *outputCursor = name;
                            while (*tagTextCursor != ':')
                                *outputCursor++ = *tagTextCursor++;
                            *outputCursor = 0;
                            labelTagCursor = tagTextCursor;
                            instStream *resolvedTargetStream = program->findStream(name);
                            if (!resolvedTargetStream) {
                                reportUserMessage("branch-to-register tag, failed to find label '%s'\n", name);
                                return false;
                            }
                            if (!program->linkStreams(currentStream, resolvedTargetStream, type)) {
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
            if (!currentStream->endDirectiveText) {
                reportError("no macro set in end_macro\n");
                return false;
            }
            /* fall through */
        case 3:
            if (currentStream->branchType) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            break;
        case 2:
            if (currentStream->branchType) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            if (!currentStream->nextStream) {
                reportUserMessage("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                return false;
            }
            if (!program->linkStreams(currentStream, currentStream->nextStream, 0)) {
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
        for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
            int btorMask = 1;
            int btor = (currentStream->branchType >> 1) & btorMask;
            if ((unsigned char)btor) {
                int changed = 0;
                if (!fixupBtorBranch(program, currentStream, &changed)) {
                    reportError("failed to fixup a branch-to-register instruction\n");
                    return false;
                }
                if (changed)
                    again = true;
            }
        }
    } while (again);

    dumpProgramStreamsAndRegisters(program, 0);
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        Instr *cleanupBranchInstruction = currentStream->getBranch();
        if (cleanupBranchInstruction && cleanupBranchInstruction->branchLabelTags) {
            delete[] cleanupBranchInstruction->branchLabelTags;
            cleanupBranchInstruction->branchLabelTags = 0;
        }
    }
    return true;
}

bool fixupBtorBranch(Program *program, instStream *stream, int *changed)
{
    *changed = 0;
    if (((stream->branchType >> 1) & 1) == 0) {
        reportError("balFix:: not reg-target\n");
        return false;
    }
    Instr *instruction = stream->getBranch();
    if (!instruction) {
        reportError("balFix-no branch!\n");
        return false;
    }
    RegPoolEntry *reg = instruction->getBranchTargetRegister();
    if (!reg) {
        reportError("no target register input for btor\n");
        instruction->leDump();
        return false;
    }
    instruction->dumpOperandDiagnostics(false);
    instStream *currentStream;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream)
        currentStream->visitState = 0;
    stream->visitState = 1;
    bool first = true;
    bool again;
    do {
        again = false;
        for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
            if (currentStream->visitState != 1)
                continue;
            workTable *scheduleTable = currentStream->getTable();
            Instr *previousInstruction;
            if (first) {
                previousInstruction = instruction->linearLink.previousInstruction;
                currentStream->visitState = 0;
                first = false;
            } else {
                previousInstruction = scheduleTable->linearInstructions.tail;
                currentStream->visitState = 2;
            }
            for (; previousInstruction; previousInstruction = previousInstruction->linearLink.previousInstruction) {
                if (previousInstruction->outputMaskForRegister(reg, 0xff)) {
                    previousInstruction->dumpOperandDiagnostics(false);
                    break;
                }
            }
            if (previousInstruction) {
                if ((previousInstruction->branchFlags >> 3) & 1) {
                    if (!currentStream->nextStream) {
                        reportUserMessage("no physical successor to a branch+link\n");
                        return false;
                    }
                    if (!program->areLinkedBy(stream, currentStream->nextStream, 2)) {
                        if (!program->linkStreams(stream, currentStream->nextStream, 2)) {
                            reportError("failed to fixup a branch to register\n");
                            return false;
                        }
                        (*changed)++;
                        again = true;
                    }
                } else if (!instruction->branchLabelTags) {
                    reportError("VCL has found a branch to register at :\n");
                    instruction->dumpOperandDiagnostics(false);
                    reportError("that's apparently set by:\n");
                    previousInstruction->dumpOperandDiagnostics(false);
                    reportError("unfortunately vcl can't figure out where the branch should go..\n");
                    reportError("try adding a tag list to the branch\n");
                    return false;
                }
            } else {
                for (linkNode *incomingLink = currentStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next) {
                    instStream *sourceStream = incomingLink->otherStream;
                    if (!sourceStream) {
                        reportError("null outist\n");
                        return false;
                    }
                    if (!sourceStream->visitState) {
                        sourceStream->visitState = 1;
                        again = true;
                    }
                }
            }
        }
    } while (again);
    return true;
}
