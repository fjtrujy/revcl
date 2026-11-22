#include "decodedLine.h"
#include "Instr.h"

int maximumDecodedLineFlowId(DecodedLine *blocks, int count)
{
    int max = 0;
    for (int lineIndex = 0; lineIndex != count; lineIndex++)
        if (blocks[lineIndex].flowId > max)
            max = blocks[lineIndex].flowId;
    return max;
}

bool DecodedLine::hasFlowStartSynthetic()
{
    for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
        Instr *instruction = instructions[instructionIndex];
        if (instruction) {
            char syntheticKind = instruction->syntheticKind;
            if (syntheticKind == 11 || syntheticKind == 3 || syntheticKind == 1)
                return true;
        }
    }
    return false;
}
