// Byte-exact reconstruction of VUC InstructionList::Add from GCC 3.3 STL.
#include <vector>
#include <ostream>
#include <cstddef>
#include "MLGameString.h"
struct Instruction {
    MLGameString text;
    int reserved;
    int kind;
    Instruction(const MLGameString &name) : text(name), kind(2) {}
    static void *operator new(std::size_t);
    const char *ToString();
};
class InstructionList {
public:
    void Add(Instruction *instruction);
    void Print(std::ostream &output);
    void AddL(const MLGameString &name);
    std::vector<Instruction *> instructions_;
};

void InstructionList::Add(Instruction *instruction)
{
    instructions_.push_back(instruction);
}

void InstructionList::Print(std::ostream &output)
{
    for (std::vector<Instruction *>::iterator it = instructions_.begin();
         it != instructions_.end(); ++it)
        output << (*it)->ToString() << std::endl;
}

void InstructionList::AddL(const MLGameString &name)
{
    Add(new Instruction(name));
}
