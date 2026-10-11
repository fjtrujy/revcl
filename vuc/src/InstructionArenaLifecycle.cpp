// Exact original Instruction arena initializer and global constructor wrapper.
// The generated local cleanup callback is still experimental (45/46 bytes).
#include <iostream>
#include <new>
class Instruction { public: static char *instrData; };
char *Instruction::instrData;
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
