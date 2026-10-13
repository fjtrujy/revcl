// VUC compiler diagnostic, scanner and main functions share static globals.
#include "../src/CurrFileInitialization.cpp"
#include "../src/ScriptLexer.cpp"
void script_error(const char *message)
{
    error = true;
    std::cout << currFileName;
    std::cout << "(" << script_line << "):"
              << message << std::endl;
}
#include "../src/Main.cpp"
