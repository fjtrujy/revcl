#ifndef MLIST_H
#define MLIST_H

#pragma interface

class mNode;

class mList {
public:
    mList();
    virtual ~mList();
    virtual bool func_0804b1b0();
    virtual mNode *func_0804b444(const char *s);
    mNode *func_0804b20c(void *data);
    mNode *func_0804b234(const char *name);
    mNode *func_0804b284(const char *name);
    mNode *func_0804b2d4(const char *name);
    bool func_0804b330(mNode *node);
    bool func_0804b38c(mNode *node);
    bool func_0804b3e8(mNode *node);
    bool func_0804b4d0(mList *other);
    void func_0804b534();
    char *func_0804b550();
    char *func_0804b574(char *buf);
    int func_0804b794();
    int func_0804b7b4();

    int unk_04;
    int unk_08;     // node count
    mNode *unk_0c;  // head
    mNode *unk_10;  // tail
};

class mNode {
public:
    mNode()
    {
        unk_04 = 0;
        unk_08 = 0;
        unk_0c = 0;
        unk_14 = 0;
        unk_10 = 0;
        unk_18 = 0;
    }
    virtual ~mNode();
    virtual bool func_0804b75c();

    bool detach();
    bool setName(const char *name);

    mNode *unk_04;  // next
    mNode *unk_08;  // prev
    mList *unk_0c;  // owning list
    void *unk_10;   // data/key
    char *unk_14;
    int unk_18;
};

#endif
