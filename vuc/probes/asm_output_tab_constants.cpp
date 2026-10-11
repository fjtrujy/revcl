// Source hypothesis for the original VUC constant-column width scan.
#include <cstring>
#include <vector>

class Instruction {
public:
    const char *ToString();
};

class InstructionList {
public:
    std::vector<Instruction *> instructions_;
};

extern int constantsTabStop;

void AsmOutput_TabConstants(InstructionList &list)
{
    int count = list.instructions_.size();
    for (int i = 0; i < count; ++i) {
        const char *text = list.instructions_[i]->ToString();
        const char *delimiter = std::strstr(text, ".assign");
        if (!delimiter)
            delimiter = std::strstr(text, ".equ");
        if (delimiter) {
            int width = delimiter - text;
            if (width > constantsTabStop)
                constantsTabStop = width;
        }
    }
}
