// C++03 source model of original odd masked-vector broadcast.
// The redundant 15 check is visible in the original ELF despite &7.
#include "../src/MLGameString.h"
namespace IR {
struct Register { MLGameString name; unsigned channels; };
}
class InstructionList { public: void Add(const char *format, ...); };
IR::Register BroadcastMove(const IR::Register &result,
                           const IR::Register &input,
                           InstructionList &output)
{
    if (result.channels & 7) {
        const char *inputName=input.name.c_str();
        const char *resultName=result.name.c_str();
        unsigned mask=result.channels & 7;
        const char *suffix="";
        if (mask != 15 && mask != 0) {
            MLTempString channels(".");
            MLTempString first((mask & 1) ? "x" : "");
            channels += first;
            MLTempString second((mask & 2) ? "y" : "");
            channels += second;
            MLTempString third((mask & 4) ? "z" : "");
            channels += third;
            MLTempString fourth("");
            channels += fourth;
            suffix=channels.text;
        }
        output.Add("add%s %s, vf00, %s", suffix, resultName, inputName);
    } else if (result.channels & 8) {
        output.Add("mul.w %s, vf00, %s", result.name.c_str(), input.name.c_str());
    }
    return result;
}
