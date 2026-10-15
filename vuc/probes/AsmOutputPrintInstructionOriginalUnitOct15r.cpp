// Experimental original AsmOutput.cpp source-unit reconstruction.
// Reuses exactly accepted predecessor bodies and authentic Instruction layout.
// The printer is still only partial; keep this file outside vuc/src.
#include "../src/AsmOutputUnit.cpp"

void AsmOutput_PrintInstruction(Instruction *instruction,std::ofstream &output)
{
    const char *text=instruction->ToString();
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
        output << "  " << AlignString(text,14) << "\n";
    }
}
