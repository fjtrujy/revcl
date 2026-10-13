// Byte-exact reconstruction of VUC InstructionList::Add from GCC 3.3 STL.
#include <vector>
#include <ostream>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include "../src/MLGameString.h"
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
    void Add(const char *format, ...);
    void AddB(const char *format, ...);
    void AddLineComment(int line);
    void Print(std::ostream &output);
    void AddL(const MLGameString &name);
    static MLGameString GenTmp(const char *name);
    static MLGameString GenLabel(const char *name);
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

MLGameString InstructionList::GenLabel(const char *name)
{
    static int id = 1;
    MLTempString temporary("L_%03d_%s\\@:", id++, name);
    return MLGameString(temporary);
}

MLGameString InstructionList::GenTmp(const char *name)
{
    static int id = 1;
    MLTempString temporary("%s_%03d", name, id++);
    return MLGameString(temporary);
}

void InstructionList::Add(const char *format, ...)
{
    char line[1024];
    va_list arguments;
    va_start(arguments, format);
    vsprintf(line, format, arguments);
    va_end(arguments);
    Instruction *instruction = new Instruction(MLGameString(line, false));
    instruction->kind = 0;
    Add(instruction);
}

void InstructionList::AddB(const char *format, ...)
{
    char line[1024];
    va_list arguments;
    va_start(arguments, format);
    vsprintf(line, format, arguments);
    va_end(arguments);
    Instruction *instruction = new Instruction(MLGameString(line, false));
    instruction->kind = 1;
    Add(instruction);
}

void InstructionList::AddLineComment(int line)
{
    if (instructions_.size()) {
        Instruction *previous = instructions_[instructions_.size() - 1];
        if (previous->kind == 3 && previous->reserved == line)
            return;
    }
    Instruction *instruction = new Instruction(MLGameString("", false));
    instruction->kind = 3;
    instruction->reserved = line;
    Add(instruction);
}
