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
    instStream *currentStream;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (currentStream->startType == 2) {
            currentStream->reachableFromEntry = 1;
            currentStream->visitState = 1;
        } else {
            currentStream->reachableFromEntry = 0;
            currentStream->visitState = 0;
        }
        if (currentStream->passThroughText) {
            reportInfo("pass thru  - setting reached top for %s\n", currentStream->streamName());
            currentStream->reachableFromEntry = 1;
        }
    }

    bool changed;
    do {
        changed = false;
        for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
            if (currentStream->visitState == 1) {
                currentStream->visitState = 2;
                for (linkNode *link = currentStream->outputLinks.head; link; link = link->next) {
                    instStream *destinationStream = link->otherStream;
                    if (!destinationStream) {
                        reportError("null outist\n");
                        return false;
                    }
                    destinationStream->reachableFromEntry = 1;
                    if (destinationStream->visitState == 0) {
                        destinationStream->visitState = 1;
                        changed = true;
                    }
                }
            }
        }
    } while (changed);

    instStream *next;
    for (currentStream = program->streamHead; currentStream; currentStream = next) {
        next = currentStream->nextStream;
        if (currentStream->reachableFromEntry)
            continue;
        reportInfo("prune inaccessible stream %s\n", currentStream->streamName());
        workTable *scheduleTable = currentStream->getTable();
        for (Instr *currentInstruction = scheduleTable->linearInstructions.head; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction)
            if (!currentInstruction->isNop()) {
                reportInfo("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                currentInstruction->leDump();
                errorOccurred = saved;
            }
        linkNode *incomingLink;
        while ((incomingLink = currentStream->inputLinks.head) != 0) {
            if (!program->unlinkStreams(incomingLink->otherStream, currentStream, incomingLink->linkType)) {
                reportError("unlink failed for input\n");
                return false;
            }
        }
        while ((incomingLink = currentStream->outputLinks.head) != 0) {
            if (!program->unlinkStreams(currentStream, incomingLink->otherStream, incomingLink->linkType)) {
                reportError("unlink failed for output\n");
                return false;
            }
        }
        if (!program->removeStream(currentStream)) {
            reportError("failed to unlink an ist\n");
            return false;
        }
        delete currentStream;
    }

    instStream *follow;
    for (currentStream = program->streamHead; currentStream; currentStream = follow) {
        follow = currentStream->nextStream;
        instStream *next = follow;
        if (next && currentStream->branchType && currentStream->getRawSchedulingMode() != 4) {
            bool keepLink = false;
            linkNode *outgoingLink;
            for (outgoingLink = currentStream->outputLinks.head; outgoingLink; outgoingLink = outgoingLink->next) {
                if (outgoingLink->otherStream != next)
                    break;
                if (!outgoingLink->linkType)
                    keepLink = true;
            }
            if (!outgoingLink) {
                reportInfo("remove branch..\n");
                Instr *branchInstruction = currentStream->getBranch();
                if (!branchInstruction) {
                    reportError("which wasn't there?\n");
                    return false;
                }
                int saved = errorOccurred;
                errorOccurred = !quietDiagnostics;
                branchInstruction->dumpOperandDiagnostics(false);
                errorOccurred = saved;
                int type = branchLinkTypeForFlags(currentStream->branchType);
                if (!program->unlinkStreams(currentStream, next, type)) {
                    reportError("failed to unlink the streams through the branch\n");
                    return false;
                }
                if (!currentStream->removeBranch()) {
                    reportError("failed to remove branch instruction\n");
                    return false;
                }
                if (!keepLink && !program->linkStreams(currentStream, next, 0)) {
                    reportError("failed to relink two streams following removing an absolute branch\n");
                    return false;
                }
            }
        }
    }
    return true;
}
