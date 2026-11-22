// Links between instruction streams (input/output link lists).
#ifndef LINKLIST_H
#define LINKLIST_H

class instStream;

struct linkNode {
    char unk_00[4];
    linkNode *previous;
    linkNode *next;
    instStream *otherStream;
    char unk_10;
    char active;
    char unk_12[2];
    int linkType;

    enum InitTag { INIT };
    linkNode();
    inline linkNode(InitTag)
    {
        linkType = -1;
        previous = 0;
        next = 0;
        otherStream = 0;
        active = 1;
    }
    ~linkNode();
    // Next node (after this one) with the given link type.
    linkNode *nextLinkOfType(int type);
    int linkedStreamId();
};

struct linkList {
    linkNode *head;
    linkNode *tail;
    int count;
    linkList();
    ~linkList();
    linkNode *findType(int type);
    instStream *getStream(int type);
    void dump();
    bool removeAll();
    bool relink(linkNode *n);
    bool unlink(linkNode *n);
    linkNode *findLinkToStream(instStream *s, int type);
    bool addNewRef(instStream *s, int type);
    int countMarkedLinks();
};

#endif
