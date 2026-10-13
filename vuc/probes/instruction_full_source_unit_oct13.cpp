// Probe genuine Instruction methods and arena lifecycle in one historical TU.
#include <iostream>
#include <new>
#include <cstddef>
#include "../src/MLGameString.h"
class Instruction {
public:
    static char *instrData;
    static void *operator new(std::size_t size);
    const char *ToString();
private:
    MLGameString text_;
};
char *Instruction::instrData;
const char *Instruction::ToString()
{
    return text_.c_str();
}
void *Instruction::operator new(std::size_t size)
{
    if (*(char **)instrData + size > instrData + 0x2004) {
        char *chunk = (char *)::operator new(0x2008);
        *(char **)chunk = chunk + 4;
        *(char **)(chunk + 0x2004) = instrData;
        instrData = chunk;
    }
    char *result = *(char **)instrData;
    *(char **)instrData = result + size;
    return result;
}
struct InstructionArenaCleanup {
    InstructionArenaCleanup() {
        char *chunk = (char *)::operator new(0x2008);
        *(char **)chunk = chunk + 4;
        *(char **)(chunk + 0x2004) = 0;
        Instruction::instrData = chunk;
    }
    ~InstructionArenaCleanup() {
        while (Instruction::instrData) {
            char *current = Instruction::instrData;
            char *next = *(char **)(current + 0x2004);
            ::operator delete(current);
            Instruction::instrData = next;
        }
    }
};
static InstructionArenaCleanup cleanup;
