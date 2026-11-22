#ifndef MLIST_H
#define MLIST_H

class mNode;

class mList {
public:
    mList();
    virtual ~mList();
    virtual bool clearNodes();
    virtual mNode *appendNamedNode(const char *s);
    mNode *findByData(void *data);
    mNode *findByExactName(const char *name);
    mNode *findByNameIgnoreCase(const char *name);
    mNode *findOrAppendByExactName(const char *name);
    bool prependNode(mNode *node);
    bool appendNode(mNode *node);
    bool detachNode(mNode *node);
    bool transferNodesFrom(mList *other);
    void noopListTraversal();
    char *firstNodeName();
    char *appendNamedLabels(char *buf);
    int combinedFlags();
    int takeCombinedFlags();

    int nodeSerial; // monotonically increasing ID assigned when a node joins
    int nodeCount;
    mNode *head;
    mNode *tail;
};

class mNode {
public:
    enum InitTag { INIT };
    mNode();
    mNode(InitTag)
    {
        next = 0;
        previous = 0;
        ownerList = 0;
        name = 0;
        data = 0;
        flags = 0;
    }
    virtual ~mNode();
    virtual bool clearNameAndOwner();

    bool detach();
    bool setName(const char *name);

    mNode *next;
    mNode *previous;
    mList *ownerList;
    void *data;     // generic pointer data or generated node key
    char *name;
    int flags;
};

#endif
