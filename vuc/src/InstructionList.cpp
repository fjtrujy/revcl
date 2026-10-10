// Byte-exact reconstruction of VUC InstructionList::Add from GCC 3.3 STL.
#include <vector>
struct Instruction;
class InstructionList {
public:
    void Add(Instruction *instruction);
    std::vector<Instruction *> instructions_;
};

void InstructionList::Add(Instruction *instruction)
{
    instructions_.push_back(instruction);
}
