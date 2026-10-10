// Reconstructed VUC AsmOutput.cpp fragment.
// Only AsmOutput_WriteEpilogue is verified byte-exact with the current
// GCC 3.3.3 source-date compiler and -O2 -frename-registers.
// The complete original translation unit and its flags remain unverified.
#include <fstream>

void AsmOutput_WriteEpilogue(std::ofstream &out)
{
    out << "\n--exit\n--endexit\n";
}
