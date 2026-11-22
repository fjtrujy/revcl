#ifndef DECODEDLINE_H
#define DECODEDLINE_H

#include "mList.h"

class Parser;
class Instr;
class InstructionStream;
struct Program;

// Decoded source-line entry produced by the parser (historical sizeof 0x28).
struct DecodedLine {
    char *passThroughText;    // owned raw source text for pass-through blocks
    char isBranchTarget;
    char decodeMode;          // UTA, RAW, pass-through, or synthetic mode
    char decodeSubMode;
    short flowId;
    short rawLoopId;
    Instr *instructions[2];
    mList labels;

    void initialize();
    void refreshInstructionOperands(int lineIndex);
    void releaseContents();
    bool hasFlowStartSynthetic();
};

void refreshDecodedLineOperands(DecodedLine *blocks, int count);
DecodedLine *findDecodedLineByLabel(DecodedLine *blocks, int count, const char *name);
int maximumDecodedLineFlowId(DecodedLine *blocks, int count);
DecodedLine *decodeSourceLines(Program *p, Parser *parser, int type, bool newSyntax, int *count);
bool splitParserStream(Program *p, DecodedLine *blocks, int count);
bool buildPassThru(Program *program, InstructionStream *s, DecodedLine *blocks, int count);
bool buildRaw(Program *program, InstructionStream *s, DecodedLine *blocks, int count, int loopMode);

#endif
