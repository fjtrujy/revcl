// Original-era formatted InstructionList insertion using a named string temporary.
#include "../src/MLGameString.h"
#include <cstdarg>
#include <cstdio>
#include <cstddef>

struct Instruction {
    MLGameString text;
    int reserved;
    int kind;
    Instruction(const MLGameString &name) : text(name), kind(0) {}
    static void *operator new(std::size_t);
};
class InstructionList {
public:
    void Add(Instruction *instruction);
    void Add(const char *format, ...);
};
void InstructionList::Add(const char *format, ...)
{
    char formatted[1024];
    va_list args;
    va_start(args, format);
    vsprintf(formatted, format, args);
    va_end(args);
    MLGameString temporary(formatted, false);
    Add(new Instruction(temporary));
}
