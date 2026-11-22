// Instruction stream reference lists ("istRefList").
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

const char *linkTypeName(int type);
bool isExportedSymbol(Program *p, char *name);
const char *getUniqueName(Program *p);
char *joinNameWithPrefix(const char *a, const char *b);

static inline StreamLinkNode *findRef(StreamLinkNode *link, InstructionStream *stream, int type)
{
    while (link && !(link->otherStream == stream && link->linkType == type))
        link = link->next;
    return link;
}

static inline int branchLinkType(unsigned int branchFlags)
{
    int resolvedLinkType = 0;
    if (branchFlags) {
        switch (branchFlags & 0xa) {
        case 8: resolvedLinkType = 3; break;
        case 2: resolvedLinkType = 2; break;
        case 10: resolvedLinkType = 4; break;
        default: resolvedLinkType = 1; break;
        }
    }
    return resolvedLinkType;
}

StreamLinkNode::StreamLinkNode()
{
    linkType = -1;
    previous = 0;
    next = 0;
    otherStream = 0;
    active = 1;
}

StreamLinkNode::~StreamLinkNode()
{
}

StreamLinkNode *StreamLinkNode::nextLinkOfType(int type)
{
    StreamLinkNode *link = this;
    do
        link = link->next;
    while (link && link->linkType != type);
    return link;
}

StreamLinkList::StreamLinkList()
{
    head = 0;
    tail = 0;
    count = 0;
}

StreamLinkList::~StreamLinkList()
{
    StreamLinkNode *link = head;
    while (link) {
        if (!unlink(head))
            return;
        if (link)
            ::operator delete(link);
        link = head;
    }
}

StreamLinkNode *StreamLinkList::findType(int type)
{
    StreamLinkNode *link = head;
    while (link && link->linkType != type)
        link = link->next;
    return link;
}

InstructionStream *StreamLinkList::getStream(int type)
{
    StreamLinkNode *found = 0;
    StreamLinkNode *link;
    for (link = head; link; link = link->next) {
        if (link->linkType == type) {
            if (found) {
                reportError("istRefList::getStream  called - but there's more than one ref\n");
                return 0;
            }
            found = link;
        }
    }
    if (found)
        return found->otherStream;
    reportError("istRefList: getStream - couldn't find type %d\n", type);
    for (StreamLinkNode *currentLink = head; currentLink; currentLink = currentLink->next)
        ;
    return 0;
}

void StreamLinkList::dump()
{
    for (StreamLinkNode *link = head; link; link = link->next)
        ;
}

bool StreamLinkList::removeAll()
{
    StreamLinkNode *link = head;
    while (link) {
        if (!unlink(head))
            return false;
        if (link)
            ::operator delete(link);
        link = head;
    }
    return true;
}

bool StreamLinkList::relink(StreamLinkNode *link)
{
    if (!link) {
        reportError("istRefLit:: relink - null ptr\n");
        return false;
    }
    if (link->next || link->previous) {
        reportError("istRefList :: relink - node is linked\n");
        return false;
    }
    StreamLinkNode *existingLink = head;
    InstructionStream *stream = link->otherStream;
    int type = link->linkType;
    while (existingLink && !(existingLink->otherStream == stream && existingLink->linkType == type))
        existingLink = existingLink->next;
    if (existingLink) {
        reportError("istRefList: already present for relink\n");
        return false;
    }
    if (tail) {
        tail->next = link;
        link->previous = tail;
    } else
        head = link;
    tail = link;
    count++;
    return true;
}

bool StreamLinkList::unlink(StreamLinkNode *link)
{
    if (!link)
        return false;
    if (link->next)
        link->next->previous = link->previous;
    else
        tail = link->previous;
    if (link->previous)
        link->previous->next = link->next;
    else
        head = link->next;
    link->previous = 0;
    link->next = 0;
    count--;
    return true;
}

StreamLinkNode *StreamLinkList::findLinkToStream(InstructionStream *stream, int type)
{
    return findRef(head, stream, type);
}

bool StreamLinkList::addNewRef(InstructionStream *stream, int type)
{
    if (findRef(head, stream, type)) {
        reportError("addNewRef - already present\n");
        return false;
    }
    StreamLinkNode *link = new StreamLinkNode(StreamLinkNode::INIT);
    if (!link) {
        reportError("addNewRef - new failed\n");
        return false;
    }
    link->linkType = type;
    link->otherStream = stream;
    if (tail) {
        tail->next = link;
        link->previous = tail;
    } else
        head = link;
    tail = link;
    count++;
    return true;
}

bool linkTypeOk(InstructionStream *stream, int type)
{
    if (type == 5)
        type = 2;
    return type == branchLinkType(stream->branchType) || (type == 0 && (stream->branchType & 4));
}

bool Program::linkStreams(InstructionStream *stream, InstructionStream *destinationStream, int type)
{
    if (!linkTypeOk(stream, type)) {
        reportError("incompatible lnktypes for %s...src=%s\n", linkTypeName(type), stream->branchTypeName());
        return false;
    }
    if (!stream->outputLinks.addNewRef(destinationStream, type)) {
        reportError("LinkStreams - add out failed\n");
        return false;
    }
    if (!destinationStream->inputLinks.addNewRef(stream, type)) {
        reportError("LinkStreams - add in failed\n");
        return false;
    }
    return true;
}

bool Program::areLinkedBy(InstructionStream *sourceStream, InstructionStream *destinationStream, int type)
{
    bool hasSD = findRef(sourceStream->outputLinks.head, destinationStream, type) != 0;
    bool hasDS = findRef(destinationStream->inputLinks.head, sourceStream, type) != 0;
    if (hasSD) {
        if (hasDS)
            return true;
    } else if (!hasDS)
        return false;
    reportError("areLinkedBy - hasSD=%d hasDS=%d src=%s dst=%s\n",
                  hasSD, hasDS, sourceStream->streamName(), destinationStream->streamName());
    exitAfterCleanup(-1);
    return false;
}

bool Program::unlinkStreams(InstructionStream *stream, InstructionStream *destinationStream, int type)
{
    StreamLinkNode *link = findRef(stream->outputLinks.head, destinationStream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      stream->streamName(), destinationStream->streamName(), type);
        for (StreamLinkNode *currentLink = stream->outputLinks.head; currentLink; currentLink = currentLink->next)
            ;
        return false;
    }
    if (!stream->outputLinks.unlink(link)) {
        reportError("unlink streams - failed to unlink out ref\n");
        return false;
    }
    link = findRef(destinationStream->inputLinks.head, stream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in input of %s\n", destinationStream->streamName());
        for (StreamLinkNode *incomingLink = destinationStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next)
            ;
        return false;
    }
    if (!destinationStream->inputLinks.unlink(link)) {
        reportError("unlink streams - failed to unlink in ref\n");
        return false;
    }
    return true;
}

bool Program::unLinkStreams(InstructionStream *stream, InstructionStream *destinationStream, int type)
{
    StreamLinkNode *link = findRef(stream->outputLinks.head, destinationStream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      stream->streamName(), destinationStream->streamName(), type);
        for (StreamLinkNode *currentLink = stream->outputLinks.head; currentLink; currentLink = currentLink->next)
            ;
        return false;
    }
    link->active = 0;
    link = findRef(destinationStream->inputLinks.head, stream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in input of %s\n", destinationStream->streamName());
        for (StreamLinkNode *incomingLink = destinationStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next)
            ;
        return false;
    }
    link->active = 0;
    return true;
}

int StreamLinkList::countMarkedLinks()
{
    int activeCount = 0;
    for (StreamLinkNode *link = head; link; link = link->next)
        if (link->active)
            activeCount++;
    return activeCount;
}

int StreamLinkNode::linkedStreamId()
{
    return otherStream ? otherStream->streamId : -1;
}

int branchLinkTypeForFlags(int flags)
{
    return branchLinkType(flags);
}

const char *linkTypeName(int type)
{
    const char *linkTypeText = "IFT_UNKNOWN";
    switch (type) {
    case -1: linkTypeText = "IFT_BAD"; break;
    case 0: linkTypeText = "IFT_FALL"; break;
    case 1: linkTypeText = "IFT_ABS"; break;
    case 2: linkTypeText = "IFT_REG"; break;
    case 3: linkTypeText = "IFT_ABS_LINK"; break;
    case 4: linkTypeText = "IFT_REG_LINK"; break;
    case 5: linkTypeText = "IFT_REG_RET"; break;
    }
    return linkTypeText;
}

