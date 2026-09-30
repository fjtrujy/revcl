// Links between instruction streams (input/output link lists).
#ifndef LINKLIST_H
#define LINKLIST_H

#pragma interface

class instStream;

struct linkNode {
    char unk_00[4];
    int unk_04;
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
    linkNode *func_0806dd48(int type)
    {
        linkNode *n = this;
        do
            n = n->unk_08;
        while (n && n->unk_14 != type);
        return n;
    }
};

struct linkList {
    linkNode *unk_00;           // head
    int unk_04;
    int unk_08;                 // count
    linkList();
    ~linkList();
    linkNode *func_0806dff4(instStream *s, int type);
    instStream *getStream(int type);
    void dump();
    bool removeAll();
    bool func_0806dfa0(linkNode *n);
    int func_0806e4f0();
};

#endif
