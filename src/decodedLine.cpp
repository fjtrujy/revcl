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
        labelList->func_0804b534();
    for (int i = 0; i != 2; i++) {
        if (instructions[i]) {
            Instr *in = instructions[i];
            in->func_08054008(false);
        }
    }
}

void refreshDecodedLineOperands(DecodedLine *blocks, int count)
{
    if (blocks)
        for (int i = 0; i != count; i++)
            blocks[i].refreshInstructionOperands(i);
}

void DecodedLine::releaseContents()
{
    labels.clearNodes();
    if (passThroughText)
        delete[] passThroughText;
    passThroughText = 0;
    for (int i = 0; i != 2; i++) {
        if (instructions[i])
            delete instructions[i];
        instructions[i] = 0;
    }
}

DecodedLine *findDecodedLineByLabel(DecodedLine *blocks, int count, const char *name)
{
    for (int i = 0; i != count; i++)
        if (blocks[i].labels.findByNameIgnoreCase(name))
            return &blocks[i];
    return 0;
}
