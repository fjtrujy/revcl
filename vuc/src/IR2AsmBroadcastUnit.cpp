// VUC original-era partial source-unit reconstruction.
// Only the accepted startup callback, not the large application body, is byte-exact.
// Source-level reconstruction of the original IR2Asm.cpp broadcast entry.
// This ordinary C++ emits actual VUC instruction names; the full function is partial.
#include <iostream>
#include "MLGameString.h"
namespace IR {
struct Register {
    MLGameString name;
    unsigned channels;
};
}
class InstructionList {
public:
    void Add(const char *format, ...);
};
IR::Register BroadcastMove(const IR::Register &result,
                           const IR::Register &input,
                           InstructionList &output)
{
    IR::Register destination(result);
    unsigned mask = result.channels & 7;
    if (mask == 0 && (result.channels & 8)) {
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else if (mask) {
        MLTempString channels(".");
        MLTempString first((mask & 1) ? "x" : "");
        channels += first;
        MLTempString second((mask & 2) ? "y" : "");
        channels += second;
        output.Add("add%s %s, vf00, %s", channels.text, result.name.c_str(), input.name.c_str());
    }
    return destination;
}
