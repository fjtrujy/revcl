// Probe original InstructionList line-comment insertion and duplicate suppression.
#include "../src/MLGameString.h"
#include <vector>
#include <cstddef>

struct Instruction {
    MLGameString text;
    int line;
    int kind;
    Instruction(const MLGameString &name) : text(name), kind(3) {}
    static void *operator new(std::size_t);
};

class InstructionList {
public:
    void Add(Instruction *instruction);
    void AddLineComment(int line);
    std::vector<Instruction *> instructions_;
};

void InstructionList::AddLineComment(int line)
{
    if (instructions_.size()) {
        Instruction *last = instructions_[instructions_.size() - 1];
        if (last->kind == 3 && last->line == line)
            return;
    }
    Instruction *instruction = new Instruction(MLGameString("", false));
    instruction->line = line;
    Add(instruction);
}
