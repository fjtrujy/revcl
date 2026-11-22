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
#define isLocTagValid func_0805082c
#define getLineTokenCount func_0805085c
#define startsWithDirective func_08050eec
#define isCommentToken func_08050f18
#define hasAssignmentMarker func_08050f48

// GASP source-location stack entry (8 bytes).
struct LocRef {
    Line *sourceLocation;
    int gaspLineNumber;
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
    char *lineText;
    int sourceLineNumber;
    char isRawMacroBlock;
    short gaspDepth;        // -1 = no expanded location stack
    LocRef gaspStack[16];
    int tokenCount;
    TokenEntry tokens[200];

    LocTag() { lineText = 0; tokenCount = 0; isRawMacroBlock = 0; gaspDepth = 0; }
    ~LocTag() {}
    char *tokenizeSourceLine(int *commentMode, char *input);
    void dumpSourceLine(int highlightedToken);
    char *tokenTextAt(int tokenIndex, char *separatorOut);
    char *tokenTextAt(int tokenIndex) { return tokenTextAt(tokenIndex, 0); }
    bool startsWithDirective();
    bool isCommentToken(int tokenIndex);
    bool hasAssignmentMarker();
    bool func_08050f7c(int i);

    bool isValid() {
        return (lineText && tokenCount) || (!lineText && !tokenCount);
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
bool isLocTagValid(LocTag *t);
int getLineTokenCount(LocTag *t);

struct Program;

// Source directive table entry (".name" -> handler).
struct Directive {
    const char *name;
    bool (*fn)(Program *, LocTag *);
};

Directive *func_080519a0(LocTag *t);

#endif
