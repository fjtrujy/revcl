#ifndef PARSER_H
#define PARSER_H

#include "Line.h"

// Loctag entry (name guessed from "initial loctag"), sizeof 0x3b0.
class LocTag {
public:
    char unk_000[0x3b0];

    ~LocTag() {}
};

// Source parser living on main's stack (CIE group 2). Name is a guess.
class Parser {
public:
    Parser();
    ~Parser();
    bool func_0804fb94(char *buf, const char *name, bool gasp, bool cpp);
    void reset();
    bool func_0804fd80(bool gasp, bool cpp);
    int func_0805030c(const char *name, int n);

    Line lines[0x2000];
    int unk_10000;          // line count
    char unk_10004[8];
    Line *unk_1000c;
    int unk_10010;
    char unk_10014[0x78];
    int unk_1008c;
    char *unk_10090;        // raw buffer
    char *unk_10094;
    int unk_10098;
    LocTag *unk_1009c;
};

#endif
