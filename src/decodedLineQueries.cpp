#include "decodedLine.h"
#include "Instr.h"

int maximumDecodedLineFlowId(DecodedLine *blocks, int count)
{
    int max = 0;
    for (int i = 0; i != count; i++)
        if (blocks[i].flowId > max)
            max = blocks[i].flowId;
    return max;
}

bool DecodedLine::hasFlowStartSynthetic()
{
    for (int i = 0; i != 2; i++) {
        Instr *in = instructions[i];
        if (in) {
            char t = in->syntheticKind;
            if (t == 11 || t == 3 || t == 1)
                return true;
        }
    }
    return false;
}
