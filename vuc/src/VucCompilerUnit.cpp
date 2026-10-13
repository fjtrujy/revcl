// Recovered VUC compiler-unit context for the original currFileName wrapper.
// Other independent fragments remain for separate exact-function regression.
#include "CurrFileInitialization.cpp"
#include "ScriptLexer.cpp"

// This diagnostic function is still a source-level hypothesis. Its genuine
// message formatting affects original GCC 3.3 register selection in this unit.
void script_error(const char *message)
{
    error = true;
    std::cout << currFileName;
    std::cout << "(" << script_line << "):"
              << message << std::endl;
}
#include "Main.cpp"
