// Reconstructed VUC Instruction.cpp fragment.
// Only Instruction::ToString() has been accepted as byte-exact so far.
#include "MLGameString.h"

class Instruction {
public:
    const char *ToString();
private:
    MLGameString text_;
};

const char *Instruction::ToString()
{
    return text_.c_str();
}
