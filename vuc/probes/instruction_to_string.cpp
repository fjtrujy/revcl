// Standalone translation-unit hypothesis for Instruction::ToString.
// Original function tail-jumps to MLGameString::c_str() at 0x08071800.
class MLGameString {
public:
    const char *c_str() const;
};

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
