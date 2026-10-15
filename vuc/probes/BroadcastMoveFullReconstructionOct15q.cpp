// Experimental reconstruction of all three original BroadcastMove channels.
// Keep source/flags outside accepted vuc/src until original 416-byte match.
#include "../src/MLGameString.h"
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
    unsigned mask = result.channels & 7;
    if (mask == 0 && (result.channels & 8)) {
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    } else if (mask) {
        MLTempString channels(".");
        MLTempString first((mask & 1) ? "x" : "");
        channels += first;
        MLTempString second((mask & 2) ? "y" : "");
        channels += second;
        MLTempString third((mask & 4) ? "z" : "");
        channels += third;
        MLTempString fourth("");
        channels += fourth;
        output.Add("add%s %s, vf00, %s", channels.text, result.name.c_str(), input.name.c_str());
    }
    return result;
}
