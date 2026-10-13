// Genuine VUC InstructionList and Instruction arena methods in original order.
#include <iostream>
#include <vector>
#include <ostream>
#include <cstddef>
#include <new>
#include "../src/MLGameString.h"
struct Instruction {
    MLGameString text;
    int reserved;
    int kind;
    static char *instrData;
    Instruction(const MLGameString &name) : text(name), kind(2) {}
    static void *operator new(std::size_t);
    const char *ToString();
};
class InstructionList {
public:
    void Add(Instruction *instruction);
    void Print(std::ostream &output);
    void AddL(const MLGameString &name);
    static MLGameString GenTmp(const char *name);
    static MLGameString GenLabel(const char *name);
    std::vector<Instruction *> instructions_;
};
void InstructionList::Add(Instruction *instruction) { instructions_.push_back(instruction); }
void InstructionList::AddL(const MLGameString &name) { Add(new Instruction(name)); }
MLGameString InstructionList::GenLabel(const char *name) {
    static int id = 1;
    MLTempString temporary("L_%03d_%s\\@:", id++, name);
    return MLGameString(temporary);
}
MLGameString InstructionList::GenTmp(const char *name) {
    static int id = 1;
    MLTempString temporary("%s_%03d", name, id++);
    return MLGameString(temporary);
}
void InstructionList::Print(std::ostream &output) {
    for (std::vector<Instruction *>::iterator it = instructions_.begin();
         it != instructions_.end(); ++it)
        output << (*it)->ToString() << std::endl;
}
char *Instruction::instrData;
const char *Instruction::ToString() { return text.c_str(); }
void *Instruction::operator new(std::size_t size) {
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
