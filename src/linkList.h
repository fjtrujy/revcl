// Links between instruction streams (input/output link lists).
#ifndef LINKLIST_H
#define LINKLIST_H

class InstructionStream;

struct StreamLinkNode {
    char reservedLinkHeaderBytes[4];
    StreamLinkNode *previous;
    StreamLinkNode *next;
    InstructionStream *otherStream;
    char includedInPathTraversal;
    char active;
    char reservedLinkStateBytes[2];
    int linkType;

    enum InitTag { INIT };
    StreamLinkNode();
    inline StreamLinkNode(InitTag)
    {
        linkType = -1;
        previous = 0;
        next = 0;
        otherStream = 0;
        active = 1;
    }
    ~StreamLinkNode();
    // Next node (after this one) with the given link type.
    StreamLinkNode *nextLinkOfType(int type);
    int linkedStreamId();
};

struct StreamLinkList {
    StreamLinkNode *head;
    StreamLinkNode *tail;
    int count;
    StreamLinkList();
    ~StreamLinkList();
    StreamLinkNode *findType(int type);
    InstructionStream *getStream(int type);
    void dump();
    bool removeAll();
    bool relink(StreamLinkNode *n);
    bool unlink(StreamLinkNode *n);
    StreamLinkNode *findLinkToStream(InstructionStream *s, int type);
    bool addNewRef(InstructionStream *s, int type);
    int countMarkedLinks();
};

#endif
