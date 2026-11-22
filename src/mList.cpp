// Intrusive doubly linked list (CIE group 1, after os.cpp).

#include "mList.h"
#include "os.h"

bool asciiCaseEqual(const char *a, const char *b);
bool func_0804c03c(const char *a, const char *b);
char *dupString(const char *s);
int mstrlen(const char *s);

static inline bool mNode_detach_inl(mNode *n)
{
    if (n->ownerList)
        return n->ownerList->detachNode(n);
    return false;
}

mList::mList()
{
    nodeSerial = 0;
    nodeCount = 0;
    head = 0;
    tail = 0;
}

mList::~mList()
{
    clearNodes();
}

bool mList::clearNodes()
{
    mNode *node = head;
    while (node) {
        mNode *next = node->next;
        if (!mNode_detach_inl(node))
            return false;
        if (!node->clearNameAndOwner())
            return false;
        node = next;
    }
    return true;
}

mNode *mList::findByData(void *data)
{
    mNode *node = head;
    while (node && node->data != data)
        node = node->next;
    return node;
}

mNode *mList::findByExactName(const char *name)
{
    mNode *node = head;
    while (node) {
        if (name) {
            if (node->name && func_0804c03c(name, node->name))
                break;
        } else if (!node->name)
            break;
        node = node->next;
    }
    return node;
}

mNode *mList::findByNameIgnoreCase(const char *name)
{
    mNode *node = head;
    while (node) {
        if (name) {
            if (node->name && asciiCaseEqual(name, node->name))
                break;
        } else if (!node->name)
            break;
        node = node->next;
    }
    return node;
}

mNode *mList::findOrAppendByExactName(const char *name)
{
    mNode *node = findByExactName(name);
    if (!node) {
        node = appendNamedNode(name);
        if (!node) {
            reportError("create add tail failed.\n");
            exitAfterCleanup(-1);
        }
    }
    return node;
}

bool mList::prependNode(mNode *node)
{
    if (!node)
        return false;
    if (node->ownerList)
        return false;
    node->previous = 0;
    node->next = 0;
    node->data = (void *)++nodeSerial;
    nodeCount++;
    node->ownerList = this;
    if (head) {
        node->next = head;
        head->previous = node;
    } else
        tail = node;
    head = node;
    return true;
}

bool mList::appendNode(mNode *node)
{
    if (!node)
        return false;
    if (node->ownerList)
        return false;
    node->previous = 0;
    node->next = 0;
    node->data = (void *)++nodeSerial;
    node->ownerList = this;
    nodeCount++;
    if (tail) {
        tail->next = node;
        node->previous = tail;
        tail = node;
    } else {
        tail = node;
        head = node;
    }
    return true;
}

bool mList::detachNode(mNode *node)
{
    if (!node)
        return false;
    if (node->ownerList != this)
        return false;
    node->ownerList = 0;
    nodeCount--;
    node->data = 0;
    if (node->next)
        node->next->previous = node->previous;
    else
        tail = node->previous;
    if (node->previous)
        node->previous->next = node->next;
    else
        head = node->next;
    return true;
}

mNode *mList::appendNamedNode(const char *name)
{
    mNode *node = new mNode(mNode::INIT);
    if (!node)
        return 0;
    if (name)
        node->name = dupString(name);
    if (!appendNode(node)) {
        delete node;
        return 0;
    }
    return node;
}

bool mList::transferNodesFrom(mList *other)
{
    mNode *node = other->head;
    while (node) {
        mNode *next = node->next;
        if (!mNode_detach_inl(node))
            return false;
        if (!appendNode(node))
            return false;
        node = next;
    }
    return true;
}

void mList::func_0804b534()
{
    for (mNode *node = head; node; node = node->next)
        ;
}

char *mList::firstNodeName()
{
    for (mNode *node = head; node; node = node->next)
        if (node->name)
            return node->name;
    return 0;
}

char *mList::appendNamedLabels(char *buf)
{
    for (mNode *node = head; node; node = node->next)
        if (node->name)
            buf += formatIntoBuffer(buf, "%s:\n", node->name);
    return buf;
}

mNode::mNode()
{
    next = 0;
    previous = 0;
    ownerList = 0;
    name = 0;
    data = 0;
    flags = 0;
}

mNode::~mNode()
{
    if (name)
        delete[] name;
    name = 0;
}


bool mNode::detach()
{
    if (ownerList)
        return ownerList->detachNode(this);
    return false;
}

bool mNode::setName(const char *name)
{
    if (this->name) {
        delete[] this->name;
        this->name = 0;
    }
    if (name) {
        this->name = new char[mstrlen(name) + 1];
        if (!this->name)
            return false;
        mstrcpy(this->name, name);
    }
    return true;
}

bool mNode::clearNameAndOwner()
{
    if (name)
        delete[] name;
    name = 0;
    ownerList = 0;
    return true;
}

int mList::combinedFlags()
{
    int flags = 0;
    for (mNode *node = head; node; node = node->next)
        flags |= node->flags;
    return flags;
}

int mList::takeCombinedFlags()
{
    int flags = 0;
    for (mNode *node = head; node; node = node->next) {
        flags |= node->flags;
        node->flags = 0;
    }
    return flags;
}
