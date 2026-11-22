// Instruction stream reference lists ("istRefList").
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

const char *linkTypeName(int type);
bool isExportedSymbol(Program *p, char *name);
const char *getUniqueName(Program *p);
char *mprefix_del(const char *a, const char *b);

static inline linkNode *findRef(linkNode *link, instStream *stream, int type)
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

linkNode::linkNode()
{
    linkType = -1;
    previous = 0;
    next = 0;
    otherStream = 0;
    active = 1;
}

linkNode::~linkNode()
{
}

linkNode *linkNode::nextLinkOfType(int type)
{
    linkNode *link = this;
    do
        link = link->next;
    while (link && link->linkType != type);
    return link;
}

linkList::linkList()
{
    head = 0;
    tail = 0;
    count = 0;
}

linkList::~linkList()
{
    linkNode *link = head;
    while (link) {
        if (!unlink(head))
            return;
        if (link)
            ::operator delete(link);
        link = head;
    }
}

linkNode *linkList::findType(int type)
{
    linkNode *link = head;
    while (link && link->linkType != type)
        link = link->next;
    return link;
}

instStream *linkList::getStream(int type)
{
    linkNode *found = 0;
    linkNode *link;
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
    for (linkNode *currentLink = head; currentLink; currentLink = currentLink->next)
        ;
    return 0;
}

void linkList::dump()
{
    for (linkNode *link = head; link; link = link->next)
        ;
}

bool linkList::removeAll()
{
    linkNode *link = head;
    while (link) {
        if (!unlink(head))
            return false;
        if (link)
            ::operator delete(link);
        link = head;
    }
    return true;
}

bool linkList::relink(linkNode *link)
{
    if (!link) {
        reportError("istRefLit:: relink - null ptr\n");
        return false;
    }
    if (link->next || link->previous) {
        reportError("istRefList :: relink - node is linked\n");
        return false;
    }
    linkNode *existingLink = head;
    instStream *stream = link->otherStream;
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

bool linkList::unlink(linkNode *link)
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

linkNode *linkList::findLinkToStream(instStream *stream, int type)
{
    return findRef(head, stream, type);
}

bool linkList::addNewRef(instStream *stream, int type)
{
    if (findRef(head, stream, type)) {
        reportError("addNewRef - already present\n");
        return false;
    }
    linkNode *link = new linkNode(linkNode::INIT);
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

bool linkTypeOk(instStream *stream, int type)
{
    if (type == 5)
        type = 2;
    return type == branchLinkType(stream->branchType) || (type == 0 && (stream->branchType & 4));
}

bool Program::linkStreams(instStream *stream, instStream *destinationStream, int type)
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

bool Program::areLinkedBy(instStream *src, instStream *dst, int type)
{
    bool hasSD = findRef(src->outputLinks.head, dst, type) != 0;
    bool hasDS = findRef(dst->inputLinks.head, src, type) != 0;
    if (hasSD) {
        if (hasDS)
            return true;
    } else if (!hasDS)
        return false;
    reportError("areLinkedBy - hasSD=%d hasDS=%d src=%s dst=%s\n",
                  hasSD, hasDS, src->streamName(), dst->streamName());
    exitAfterCleanup(-1);
    return false;
}

bool Program::unlinkStreams(instStream *stream, instStream *destinationStream, int type)
{
    linkNode *link = findRef(stream->outputLinks.head, destinationStream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      stream->streamName(), destinationStream->streamName(), type);
        for (linkNode *currentLink = stream->outputLinks.head; currentLink; currentLink = currentLink->next)
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
        for (linkNode *incomingLink = destinationStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next)
            ;
        return false;
    }
    if (!destinationStream->inputLinks.unlink(link)) {
        reportError("unlink streams - failed to unlink in ref\n");
        return false;
    }
    return true;
}

bool Program::unLinkStreams(instStream *stream, instStream *destinationStream, int type)
{
    linkNode *link = findRef(stream->outputLinks.head, destinationStream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      stream->streamName(), destinationStream->streamName(), type);
        for (linkNode *currentLink = stream->outputLinks.head; currentLink; currentLink = currentLink->next)
            ;
        return false;
    }
    link->active = 0;
    link = findRef(destinationStream->inputLinks.head, stream, type);
    if (!link) {
        reportError("unLinkStreams::failed to find ref for unlink in input of %s\n", destinationStream->streamName());
        for (linkNode *incomingLink = destinationStream->inputLinks.head; incomingLink; incomingLink = incomingLink->next)
            ;
        return false;
    }
    link->active = 0;
    return true;
}

int linkList::countMarkedLinks()
{
    int activeCount = 0;
    for (linkNode *link = head; link; link = link->next)
        if (link->active)
            activeCount++;
    return activeCount;
}

int linkNode::linkedStreamId()
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

