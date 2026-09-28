#ifndef PARSER_H
#define PARSER_H

#include "Line.h"

// Loctag stack entry (8 bytes); layout from Parser::func_0804fb94.
struct LocRef {
    Line *unk_00;
    int unk_04;
};

// Line table entry (probably "lineEntry", see Tokenise error string), sizeof 0x3b0.
class LocTag {
public:
    char *unk_00;           // line data
    int unk_04;             // source line number
    char unk_08;            // 1 = raw macro block
    short unk_0a;           // loctag depth, -1 = none
    LocRef unk_0c[16];
    int unk_8c;
    char unk_090[0x320];

    LocTag() { unk_00 = 0; unk_8c = 0; unk_08 = 0; unk_0a = 0; }
    ~LocTag() {}
    char *func_0805086c(int *commentMode, char *in);
    void func_08050c40(int);
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
    int func_0805030c(const char *name, int n);
    bool func_0805041c(char *line);

    Line lines[0x2000];
    int unk_10000;          // line count
    char unk_10004[8];
    LocRef unk_1000c[16];
    int unk_1008c;
    char *unk_10090;        // raw buffer
    char *unk_10094;
    int unk_10098;
    LocTag *unk_1009c;
};

#endif
