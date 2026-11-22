#include "decodedLine.h"
#include "Instr.h"

void DecodedLine::initialize()
{
    isBranchTarget = 0;
    decodeMode = 0;
    rawLoopId = 0;
    decodeSubMode = 0;
    flowId = -1;
    labels.clearNodes();
    passThroughText = 0;
    instructions[0] = 0;
    instructions[1] = 0;
}

void DecodedLine::refreshInstructionOperands(int index)
{
    mList *labelList = &labels;
    if (labelList->nodeCount)
        labelList->noopListTraversal();
    for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
        if (instructions[instructionIndex]) {
            Instr *instruction = instructions[instructionIndex];
            instruction->dumpOperandDiagnostics(false);
        }
    }
}

void refreshDecodedLineOperands(DecodedLine *blocks, int count)
{
    if (blocks)
        for (int lineIndex = 0; lineIndex != count; lineIndex++)
            blocks[lineIndex].refreshInstructionOperands(lineIndex);
}

void DecodedLine::releaseContents()
{
    labels.clearNodes();
    if (passThroughText)
        delete[] passThroughText;
    passThroughText = 0;
    for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
        if (instructions[instructionIndex])
            delete instructions[instructionIndex];
        instructions[instructionIndex] = 0;
    }
}

DecodedLine *findDecodedLineByLabel(DecodedLine *blocks, int count, const char *name)
{
    for (int lineIndex = 0; lineIndex != count; lineIndex++)
        if (blocks[lineIndex].labels.findByNameIgnoreCase(name))
            return &blocks[lineIndex];
    return 0;
}
