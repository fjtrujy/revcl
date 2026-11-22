#include "os.h"

#include "backTrace_shared.h"

bool processActiveList(Program *program)
{
    program->matchedStreams = 0;
    while (program->activeStreams) {
        instStream *stream = program->activeStreams;
        program->activeStreams = stream->nextActiveStream;
        if (stream->visitState != 1) {
            reportError("an inconsistency in the active list?\n");
            return false;
        }
        if (stream->outputLocationCount != 0) {
            stream->visitState = 3;
            if (stream->nextMatchedStream) {
                reportError("new match is already set\n");
                return false;
            }
            stream->nextMatchedStream = program->matchedStreams;
            program->matchedStreams = stream;
            if (stream->nextSourceStream) {
                reportError("new match is already set\n");
                return false;
            }
            stream->nextSourceStream = program->sourceStreams;
            program->sourceStreams = stream;
        } else {
            linkNode *head = stream->inputLinks.head;
            if (head) {
                stream->visitState = 2;
                if (!extendBackTrace(program, stream)) {
                    reportError("failed to extend back trace at %s\n", stream->streamName());
                    return false;
                }
            } else {
                stream->visitState = 4;
            }
        }
    }
    return true;
}

bool extendBackTrace(Program *program, instStream *targetStream)
{
    int cnt = 0;
    if (targetStream->visitState != 2 && targetStream->visitState != 0) {
        reportError("extend: called - but not visited or unvisited\n");
        return false;
    }
    linkNode *incomingLink;
    for (incomingLink = targetStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next) {
        if (incomingLink->linkType == 3 || incomingLink->linkType == 4) {
            if (incomingLink->includedInPathTraversal)
                break;
            cnt++;
        }
    }
    bool all = false;
    if (cnt && !incomingLink)
        all = true;

    for (incomingLink = targetStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next) {
        int branch = incomingLink->linkType == 2 || incomingLink->linkType == 4 || incomingLink->linkType == 5;
        if ((char)branch) {
            instStream *prev = targetStream->previousStream;
            if (!prev) {
                reportError("to register branch had no previous\n");
                return false;
            }
            for (linkNode *outgoingLink = prev->outputLinks.head; outgoingLink; outgoingLink = outgoingLink->next) {
                instStream *destinationStream = outgoingLink->otherStream;
                int type = outgoingLink->linkType;
                if (!destinationStream) {
                    reportError("no dest stream set for and link\n");
                    return false;
                }
                if (destinationStream->visitState == 2 && prev->visitState == 0) {
                    if (!addNewActive(program, prev, targetStream)) {
                        reportError("failed to add a new active node for predecessor in bal case\n");
                        return false;
                    }
                    if (program->traceStoreEnabled && mergeTstores(program, prev->traceStore, targetStream->traceStore) < 0) {
                        reportError("failed to merge tstores\n");
                        return false;
                    }
                    type = outgoingLink->linkType;
                }
                linkNode *back = destinationStream->inputLinks.findLinkToStream(prev, type);
                if (!back) {
                    reportError("destination had no link back\n");
                    return false;
                }
                back->includedInPathTraversal = 1;
            }
        }
        if (all || incomingLink->includedInPathTraversal) {
            instStream *sourceStream = incomingLink->otherStream;
            if (!sourceStream) {
                reportError("can't get input stream for ir during trace\n");
                return false;
            }
            if (program->traceStoreEnabled && mergeTstores(program, sourceStream->traceStore, targetStream->traceStore) < 0) {
                reportError("failed to merge tstores\n");
                return false;
            }
            switch (sourceStream->visitState) {
            case 0:
                if (!addNewActive(program, sourceStream, targetStream)) {
                    reportError("failedt to add a new active\n");
                    return false;
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4: {
                int dmin = sourceStream->minimumPathLength;
                if (dmin < 0) {
                    reportError("think I've already been here, but there's no distance to target\n");
                    return false;
                }
                int len = targetStream->minimumPathLength + targetStream->backtraceDependencyLength;
                if (len < dmin) {
                    sourceStream->shortestPathPredecessor = targetStream;
                    if (!recalcMinTraceLengths(program)) {
                        reportError("failed to recalc trace lengths\n");
                        return false;
                    }
                    if (sourceStream->minimumPathLength != len) {
                        reportError("after recalculating the minimal path lengths, got an unexpected result\n");
                        return false;
                    }
                }
                if (program->trackMaximumPathLength) {
                    int dmax = sourceStream->maximumPathLength;
                    if (dmax < 0) {
                        reportError("think I've already been here, but there's no maximal distance to target\n");
                        return false;
                    }
                    bool notOnPath = true;
                    for (instStream *pathStream = targetStream; pathStream; pathStream = pathStream->longestPathPredecessor) {
                        if (pathStream == sourceStream) {
                            notOnPath = false;
                            break;
                        }
                    }
                    if (notOnPath) {
                        int len2 = targetStream->maximumPathLength + targetStream->backtraceDependencyLength;
                        if (len2 > dmax) {
                            sourceStream->longestPathPredecessor = targetStream;
                            if (!recalcMaxTraceLengths(program)) {
                                reportError("failed to recalc trace lengths\n");
                                return false;
                            }
                            if (sourceStream->maximumPathLength != len2) {
                                reportError("after recalculating the maximal path lengths, got an unexpected result\n");
                                return false;
                            }
                        }
                    }
                }
                break;
            }
            default:
                reportError("bad visit mode\n");
                return false;
            }
        }
    }
    return true;
}

bool addNewActive(Program *program, instStream *stream, instStream *from)
{
    if (stream->visitState) {
        reportError("addNewActive called - but ist_n(%s) is not unvisited.ist_o is %s\n",
                      stream->streamName(), from->streamName());
        return false;
    }
    if (stream->nextActiveStream) {
        reportError("can't add to active list\n");
        return false;
    }
    stream->nextActiveStream = program->activeStreams;
    program->activeStreams = stream;
    stream->visitState = 1;
    stream->shortestPathPredecessor = from;
    stream->minimumPathLength = from->minimumPathLength + from->backtraceDependencyLength;
    if (program->trackMaximumPathLength) {
        stream->longestPathPredecessor = from;
        stream->maximumPathLength = from->maximumPathLength + from->backtraceDependencyLength;
    }
    return true;
}

bool recalcMinTraceLengths(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->visitState)
            continue;
        int len = 0;
        for (instStream *predecessorStream = stream->shortestPathPredecessor; predecessorStream; predecessorStream = predecessorStream->shortestPathPredecessor)
            len += predecessorStream->backtraceDependencyLength;
        if (stream->minimumPathLength < len) {
            reportError("recalcPaths - code produced a longer path.. but should be tracking shortest ..path is %s nLen=%d, but old length was %d!\n",
                          stream->streamName(), len, stream->minimumPathLength);
            return false;
        }
        stream->minimumPathLength = len;
    }
    return true;
}

bool recalcMaxTraceLengths(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->visitState)
            continue;
        int len = 0;
        for (instStream *predecessorStream = stream->longestPathPredecessor; predecessorStream; predecessorStream = predecessorStream->longestPathPredecessor)
            len += predecessorStream->backtraceDependencyLength;
        if (stream->maximumPathLength > len) {
            reportError("recalcPaths - code produced a shorter maximal path.. but should be tracking longest ..path is %s nLen=%d, but old length was %d!\n",
                          stream->streamName(), len, stream->maximumPathLength);
            return false;
        }
        stream->maximumPathLength = len;
    }
    return true;
}

