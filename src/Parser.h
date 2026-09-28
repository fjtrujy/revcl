#ifndef PARSER_H
#define PARSER_H

// Source parser living on main's stack (CIE group 2). Name is a guess.
class Parser {
public:
    Parser();
    ~Parser();
    bool func_0804fb94(char *buf, const char *name, bool gasp, bool cpp);

    char unk_00[0x1009c];
};

#endif
