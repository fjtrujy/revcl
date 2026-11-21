// Links between instruction streams (input/output link lists).
#ifndef LINKLIST_H
#define LINKLIST_H

#pragma interface

class instStream;

struct linkNode {
    char unk_00[4];
    linkNode *unk_04;           // prev
    linkNode *unk_08;           // next
    instStream *unk_0c;         // other stream
    char unk_10;                // marked
    char unk_11;
    char unk_12[2];
    int unk_14;                 // link type

    linkNode()
    {
        unk_14 = -1;
        unk_04 = 0;
        unk_08 = 0;
        unk_0c = 0;
        unk_11 = 1;
    }
    ~linkNode()
    {
    }
    // Next node (after this one) with the given link type.
    linkNode *func_0806dd48(int type);
    int func_0806e514();
};

struct linkList {
    linkNode *unk_00;           // head
    linkNode *unk_04;           // tail
    int unk_08;                 // count
    linkList();
    ~linkList();
    linkNode *findType(int type);
    instStream *getStream(int type);
    void dump();
    bool removeAll();
    bool relink(linkNode *n);
    bool unlink(linkNode *n);
    linkNode *func_0806dff4(instStream *s, int type);
    bool addNewRef(instStream *s, int type);
    int func_0806e4f0();
};

#endif
