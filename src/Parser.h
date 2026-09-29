#ifndef PARSER_H
#define PARSER_H

#include "Line.h"

// Loctag stack entry (8 bytes); layout from Parser::func_0804fb94.
struct LocRef {
    Line *unk_00;
    int unk_04;
};

// Token entry of a lineEntry, 4 bytes.
struct TokenEntry {
    unsigned short unk_00;  // offset of token text from LocTag::unk_00
    char unk_02;            // separator after token: 0 = space, 1 = comma, 2 = end of line
    bool unk_03;            // token started inside a comment
};

// Line table entry (probably "lineEntry", see Tokenise error string), sizeof 0x3b0.
class LocTag {
public:
    char *unk_00;           // line data
    int unk_04;             // source line number
    char unk_08;            // 1 = raw macro block
    short unk_0a;           // loctag depth, -1 = none
    LocRef unk_0c[16];
    int unk_8c;             // token count
    TokenEntry unk_090[200];

    LocTag() { unk_00 = 0; unk_8c = 0; unk_08 = 0; unk_0a = 0; }
    ~LocTag() {}
    char *func_0805086c(int *commentMode, char *in);
    void func_08050c40(int);
    char *func_08050be4(int i, char *type);
    bool func_08050eec();
    bool func_08050f18(int i);
    bool func_08050f48();
    bool func_08050f7c(int i);

    bool isValid() {
        return (unk_00 && unk_8c) || (!unk_00 && !unk_8c);
    }
    bool inComment(int i) {
        if (i >= 0 && i < unk_8c) {
            if (unk_090[i].unk_03)
                return true;
            return false;
        }
        return true;
    }
};

// Source parser living on main's stack (CIE group 2). Name is a guess.
class Parser {
public:
    Parser();
    ~Parser();
    bool func_0804fb94(char *buf, const char *name, bool gasp, bool cpp);
    void reset();
    bool func_0804fd80(bool gasp, bool cpp);
    int func_0805017c();
    LocTag *func_0805018c(int i);
    void Dump();
    void DumpLocTags();
    int func_0805030c(const char *name, int n);
    int FindLocTag(const char *name, int n);
    bool func_0805041c(char *line);
    bool func_08050738(int i);

    Line lines[0x2000];
    int unk_10000;          // line count
    char *unk_10004;        // name of last ".include" seen (loctag kind 1)
    char *unk_10008;        // name of last macro label seen (loctag kind 2)
    LocRef unk_1000c[16];
    int unk_1008c;
    char *unk_10090;        // raw buffer
    char *unk_10094;
    int unk_10098;
    LocTag *unk_1009c;
};

bool func_080506e0(const char *s);
bool func_08050708(char c);
bool func_08050780(const char *s, const char *kw);
bool func_080507f4(const char *s);
bool func_08050810(const char *s);
bool func_0805082c(LocTag *t);
int func_0805085c(LocTag *t);

#endif
