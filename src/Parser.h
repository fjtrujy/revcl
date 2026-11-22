#ifndef PARSER_H
#define PARSER_H

#include "Line.h"

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

bool hasTrailingLabelColon(const char *s);
bool isTokenSeparatorCharacter(char c);
bool matchesLeadingKeyword(const char *s, const char *kw);
bool matchesFirstSpecialKeyword(const char *s);
bool matchesSecondSpecialKeyword(const char *s);
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
