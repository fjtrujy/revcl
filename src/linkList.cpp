// Instruction stream reference lists ("istRefList").
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

const char *linkTypeName(int type);
bool isExportedSymbol(Program *p, char *name);
const char *getUniqueName(Program *p);
char *mprefix_del(const char *a, const char *b);

static inline linkNode *findRef(linkNode *p, instStream *s, int type)
{
    while (p && !(p->otherStream == s && p->linkType == type))
        p = p->next;
    return p;
}

static inline int branchLinkType(unsigned int f)
{
    int t = 0;
    if (f) {
        switch (f & 0xa) {
        case 8: t = 3; break;
        case 2: t = 2; break;
        case 10: t = 4; break;
        default: t = 1; break;
        }
    }
    return t;
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
    linkNode *n = this;
    do
        n = n->next;
    while (n && n->linkType != type);
    return n;
}

linkList::linkList()
{
    head = 0;
    tail = 0;
    count = 0;
}

linkList::~linkList()
{
    linkNode *n = head;
    while (n) {
        if (!unlink(head))
            return;
        if (n)
            ::operator delete(n);
        n = head;
    }
}

linkNode *linkList::findType(int type)
{
    linkNode *n = head;
    while (n && n->linkType != type)
        n = n->next;
    return n;
}

instStream *linkList::getStream(int type)
{
    linkNode *found = 0;
    linkNode *n;
    for (n = head; n; n = n->next) {
        if (n->linkType == type) {
            if (found) {
                reportError("istRefList::getStream  called - but there's more than one ref\n");
                return 0;
            }
            found = n;
        }
    }
    if (found)
        return found->otherStream;
    reportError("istRefList: getStream - couldn't find type %d\n", type);
    for (linkNode *p = head; p; p = p->next)
        ;
    return 0;
}

void linkList::dump()
{
    for (linkNode *n = head; n; n = n->next)
        ;
}

bool linkList::removeAll()
{
    linkNode *n = head;
    while (n) {
        if (!unlink(head))
            return false;
        if (n)
            ::operator delete(n);
        n = head;
    }
    return true;
}

bool linkList::relink(linkNode *n)
{
    if (!n) {
        reportError("istRefLit:: relink - null ptr\n");
        return false;
    }
    if (n->next || n->previous) {
        reportError("istRefList :: relink - node is linked\n");
        return false;
    }
    linkNode *p = head;
    instStream *s = n->otherStream;
    int type = n->linkType;
    while (p && !(p->otherStream == s && p->linkType == type))
        p = p->next;
    if (p) {
        reportError("istRefList: already present for relink\n");
        return false;
    }
    if (tail) {
        tail->next = n;
        n->previous = tail;
    } else
        head = n;
    tail = n;
    count++;
    return true;
}

bool linkList::unlink(linkNode *n)
{
    if (!n)
        return false;
    if (n->next)
        n->next->previous = n->previous;
    else
        tail = n->previous;
    if (n->previous)
        n->previous->next = n->next;
    else
        head = n->next;
    n->previous = 0;
    n->next = 0;
    count--;
    return true;
}

linkNode *linkList::findLinkToStream(instStream *s, int type)
{
    return findRef(head, s, type);
}

bool linkList::addNewRef(instStream *s, int type)
{
    if (findRef(head, s, type)) {
        reportError("addNewRef - already present\n");
        return false;
    }
    linkNode *n = new linkNode(linkNode::INIT);
    if (!n) {
        reportError("addNewRef - new failed\n");
        return false;
    }
    n->linkType = type;
    n->otherStream = s;
    if (tail) {
        tail->next = n;
        n->previous = tail;
    } else
        head = n;
    tail = n;
    count++;
    return true;
}

bool linkTypeOk(instStream *s, int type)
{
    if (type == 5)
        type = 2;
    return type == branchLinkType(s->branchType) || (type == 0 && (s->branchType & 4));
}

bool Program::linkStreams(instStream *a, instStream *b, int type)
{
    if (!linkTypeOk(a, type)) {
        reportError("incompatible lnktypes for %s...src=%s\n", linkTypeName(type), a->branchTypeName());
        return false;
    }
    if (!a->outputLinks.addNewRef(b, type)) {
        reportError("LinkStreams - add out failed\n");
        return false;
    }
    if (!b->inputLinks.addNewRef(a, type)) {
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

bool Program::unlinkStreams(instStream *a, instStream *b, int type)
{
    linkNode *n = findRef(a->outputLinks.head, b, type);
    if (!n) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      a->streamName(), b->streamName(), type);
        for (linkNode *p = a->outputLinks.head; p; p = p->next)
            ;
        return false;
    }
    if (!a->outputLinks.unlink(n)) {
        reportError("unlink streams - failed to unlink out ref\n");
        return false;
    }
    n = findRef(b->inputLinks.head, a, type);
    if (!n) {
        reportError("unLinkStreams::failed to find ref for unlink in input of %s\n", b->streamName());
        for (linkNode *p = b->inputLinks.head; p; p = p->next)
            ;
        return false;
    }
    if (!b->inputLinks.unlink(n)) {
        reportError("unlink streams - failed to unlink in ref\n");
        return false;
    }
    return true;
}

bool Program::unLinkStreams(instStream *a, instStream *b, int type)
{
    linkNode *n = findRef(a->outputLinks.head, b, type);
    if (!n) {
        reportError("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      a->streamName(), b->streamName(), type);
        for (linkNode *p = a->outputLinks.head; p; p = p->next)
            ;
        return false;
    }
    n->active = 0;
    n = findRef(b->inputLinks.head, a, type);
    if (!n) {
        reportError("unLinkStreams::failed to find ref for unlink in input of %s\n", b->streamName());
        for (linkNode *p = b->inputLinks.head; p; p = p->next)
            ;
        return false;
    }
    n->active = 0;
    return true;
}

int linkList::countMarkedLinks()
{
    int n = 0;
    for (linkNode *p = head; p; p = p->next)
        if (p->active)
            n++;
    return n;
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
    const char *s = "IFT_UNKNOWN";
    switch (type) {
    case -1: s = "IFT_BAD"; break;
    case 0: s = "IFT_FALL"; break;
    case 1: s = "IFT_ABS"; break;
    case 2: s = "IFT_REG"; break;
    case 3: s = "IFT_ABS_LINK"; break;
    case 4: s = "IFT_REG_LINK"; break;
    case 5: s = "IFT_REG_RET"; break;
    }
    return s;
}

