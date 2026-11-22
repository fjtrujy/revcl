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

bool extendBackTrace(Program *program, instStream *n)
{
    int cnt = 0;
    if (n->visitState != 2 && n->visitState != 0) {
        reportError("extend: called - but not visited or unvisited\n");
        return false;
    }
    linkNode *l;
    for (l = n->inputLinks.head; l; l = l->next) {
        if (l->linkType == 3 || l->linkType == 4) {
            if (l->includedInPathTraversal)
                break;
            cnt++;
        }
    }
    bool all = false;
    if (cnt && !l)
        all = true;

    for (l = n->inputLinks.head; l; l = l->next) {
        int branch = l->linkType == 2 || l->linkType == 4 || l->linkType == 5;
        if ((char)branch) {
            instStream *prev = n->previousStream;
            if (!prev) {
                reportError("to register branch had no previous\n");
                return false;
            }
            for (linkNode *b = prev->outputLinks.head; b; b = b->next) {
                instStream *d = b->otherStream;
                int type = b->linkType;
                if (!d) {
                    reportError("no dest stream set for and link\n");
                    return false;
                }
                if (d->visitState == 2 && prev->visitState == 0) {
                    if (!addNewActive(program, prev, n)) {
                        reportError("failed to add a new active node for predecessor in bal case\n");
                        return false;
                    }
                    if (program->traceStoreEnabled && mergeTstores(program, prev->traceStore, n->traceStore) < 0) {
                        reportError("failed to merge tstores\n");
                        return false;
                    }
                    type = b->linkType;
                }
                linkNode *back = d->inputLinks.findLinkToStream(prev, type);
                if (!back) {
                    reportError("destination had no link back\n");
                    return false;
                }
                back->includedInPathTraversal = 1;
            }
        }
        if (all || l->includedInPathTraversal) {
            instStream *o = l->otherStream;
            if (!o) {
                reportError("can't get input stream for ir during trace\n");
                return false;
            }
            if (program->traceStoreEnabled && mergeTstores(program, o->traceStore, n->traceStore) < 0) {
                reportError("failed to merge tstores\n");
                return false;
            }
            switch (o->visitState) {
            case 0:
                if (!addNewActive(program, o, n)) {
                    reportError("failedt to add a new active\n");
                    return false;
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4: {
                int dmin = o->minimumPathLength;
                if (dmin < 0) {
                    reportError("think I've already been here, but there's no distance to target\n");
                    return false;
                }
                int len = n->minimumPathLength + n->backtraceDependencyLength;
                if (len < dmin) {
                    o->shortestPathPredecessor = n;
                    if (!recalcMinTraceLengths(program)) {
                        reportError("failed to recalc trace lengths\n");
                        return false;
                    }
                    if (o->minimumPathLength != len) {
                        reportError("after recalculating the minimal path lengths, got an unexpected result\n");
                        return false;
                    }
                }
                if (program->trackMaximumPathLength) {
                    int dmax = o->maximumPathLength;
                    if (dmax < 0) {
                        reportError("think I've already been here, but there's no maximal distance to target\n");
                        return false;
                    }
                    bool notOnPath = true;
                    for (instStream *q = n; q; q = q->longestPathPredecessor) {
                        if (q == o) {
                            notOnPath = false;
                            break;
                        }
                    }
                    if (notOnPath) {
                        int len2 = n->maximumPathLength + n->backtraceDependencyLength;
                        if (len2 > dmax) {
                            o->longestPathPredecessor = n;
                            if (!recalcMaxTraceLengths(program)) {
                                reportError("failed to recalc trace lengths\n");
                                return false;
                            }
                            if (o->maximumPathLength != len2) {
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
        for (instStream *q = stream->shortestPathPredecessor; q; q = q->shortestPathPredecessor)
            len += q->backtraceDependencyLength;
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
        for (instStream *q = stream->longestPathPredecessor; q; q = q->longestPathPredecessor)
            len += q->backtraceDependencyLength;
        if (stream->maximumPathLength > len) {
            reportError("recalcPaths - code produced a shorter maximal path.. but should be tracking longest ..path is %s nLen=%d, but old length was %d!\n",
                          stream->streamName(), len, stream->maximumPathLength);
            return false;
        }
        stream->maximumPathLength = len;
    }
    return true;
}

