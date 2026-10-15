// Initial ELF-grounded C++03 AsmOutput_PrintInstruction source reconstruction.
// Keep under probes: neither size nor complete code is yet authenticated.
#include <fstream>
#include <cstring>
#include "../src/MLGameString.h"
struct Instruction {
    MLGameString text;
    int line;
    int kind;
    char *ToString();
};
extern int constantsTabStop;
char *AlignString(const char *,int);

void AsmOutput_PrintInstruction(Instruction *instruction,std::ofstream &output)
{
    char *text=instruction->ToString();
    bool isLabel=instruction->kind==2;
    bool hasInclude=std::strstr(text,".include")!=0;
    bool hasMacro=std::strstr(text,".macro")!=0;
    bool hasEndMacro=std::strstr(text,".endm")!=0;
    bool isConstant=false;
    if (std::strstr(text,".assign")) isConstant=true;
    else if (std::strstr(text,".equ")) isConstant=true;
    if (hasEndMacro)
        output << "\n";
    if (isConstant) {
        char *aligned=AlignString(text,constantsTabStop);
        char *hex=std::strstr(aligned,"0x");
        if (hex) { hex[0]='H'; hex[1]='\''; }
        output << aligned << "\n";
    } else if (isLabel || hasInclude || hasMacro || hasEndMacro) {
        output << text << "\n";
    } else {
        output << AlignString(text,14) << "  " << "\n";
    }
}
