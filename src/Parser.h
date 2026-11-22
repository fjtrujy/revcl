#ifndef PARSER_H
#define PARSER_H

#include "Line.h"

// Source-level names; retain the recovered historical C++ mangled symbols.
#define initializeParser func_0804fb94
#define buildProcessedLines func_0804fd80
#define getParsedLineCount func_0805017c
#define getParsedLine func_0805018c
#define addLocationTag func_0805030c
#define decodeGaspLine func_0805041c
#define isAssignmentLine func_08050738
#define tokenizeSourceLine func_0805086c
#define tokenTextAt func_08050be4
#define dumpSourceLine func_08050c40

// Loctag stack entry (8 bytes); layout from Parser::func_0804fb94.
struct LocRef {
    Line *unk_00;
    int unk_04;
};

// Token entry of a lineEntry, 4 bytes.
struct TokenEntry {
    unsigned short textOffset; // offset of token text within LocTag's line data
    char separator;            // 0 = space, 1 = comma, 2 = end of line
    bool inComment;            // token started inside a comment
};

// Line table entry (probably "lineEntry", see Tokenise error string), sizeof 0x3b0.
class LocTag {
public:
    char *unk_00;           // line data
    int unk_04;             // source line number
    char unk_08;            // 1 = raw macro block
    short unk_0a;           // loctag depth, -1 = none
    LocRef unk_0c[16];
    int tokenCount;
    TokenEntry tokens[200];

    LocTag() { unk_00 = 0; tokenCount = 0; unk_08 = 0; unk_0a = 0; }
    ~LocTag() {}
    char *tokenizeSourceLine(int *commentMode, char *input);
    void dumpSourceLine(int highlightedToken);
    char *tokenTextAt(int tokenIndex, char *separatorOut);
    char *tokenTextAt(int tokenIndex) { return tokenTextAt(tokenIndex, 0); }
    bool func_08050eec();
    bool func_08050f18(int i);
    bool func_08050f48();
    bool func_08050f7c(int i);

    bool isValid() {
        return (unk_00 && tokenCount) || (!unk_00 && !tokenCount);
    }
    bool inComment(int tokenIndex) {
        if (tokenIndex >= 0 && tokenIndex < tokenCount) {
            if (tokens[tokenIndex].inComment)
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
    bool initializeParser(char *buf, const char *name, bool gasp, bool cpp);
    void reset();
    bool buildProcessedLines(bool gasp, bool cpp);
    int getParsedLineCount();
    LocTag *getParsedLine(int i);
    void Dump();
    void DumpLocTags();
    int addLocationTag(const char *name, int n);
    int FindLocTag(const char *name, int n);
    bool decodeGaspLine(char *line);
    bool isAssignmentLine(int i);

    Line lines[0x2000];
    int locationTagCount;        // unique source locations in lines[]
    char *pendingIncludeName;    // last ".include" name (location kind 1)
    char *pendingMacroName;      // last macro label (location kind 2)
    LocRef gaspLocationStack[16];
    int gaspLocationDepth;
    char *rawBuffer;
    char *processedBuffer;
    int parsedLineCount;
    LocTag *parsedLines;
};

bool func_080506e0(const char *s);
bool func_08050708(char c);
bool func_08050780(const char *s, const char *kw);
bool func_080507f4(const char *s);
bool func_08050810(const char *s);
bool func_0805082c(LocTag *t);
int func_0805085c(LocTag *t);

struct Program;

// Source directive table entry (".name" -> handler).
struct Directive {
    const char *name;
    bool (*fn)(Program *, LocTag *);
};

Directive *func_080519a0(LocTag *t);

#endif
