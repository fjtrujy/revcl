// Source-level hypothesis for original InlineAsm.cpp compiler context.
// Real original APIs and parser storage; neither application body is accepted.
#include <iostream>
#include <vector>
#include <cstring>
#include "../src/MLGameString.h"
struct AsmVariable { unsigned offset; void *symbol; };
struct ASTAsmStmt {
    unsigned header[3];
    MLGameString source;
    std::vector<AsmVariable> variables;
};
struct SymbolTableEntry;
struct SymbolTable {
    SymbolTableEntry *Find(const MLGameString &, bool);
};
struct InstructionList { void Add(void *); };
extern unsigned currentPosition;
extern unsigned wordPosition;
extern char asmLine[1024];
extern char word[1024];
const char *GetNextWord(const char *);
void BindInlineAsm(ASTAsmStmt *stmt, SymbolTable &symbols)
{
    const char *input = stmt->source.c_str();
    currentPosition = 0;
    std::memset(asmLine, 0, sizeof(asmLine));
    std::memset(word, 0, sizeof(word));
    while (*input) {
        const char *next = GetNextWord(input);
        if (word[0]) {
            MLGameString name(word, false);
            SymbolTableEntry *entry = symbols.Find(name, false);
            if (entry) {
                AsmVariable variable = {wordPosition, entry};
                stmt->variables.push_back(variable);
            }
        }
        input = next;
        if (*input) ++input;
    }
}
void GenInlineAsm(MLGameString &source, std::vector<AsmVariable> &vars,
                  InstructionList &instructions)
{
    const char *text = source.c_str();
    for (std::vector<AsmVariable>::iterator it = vars.begin();
         it != vars.end(); ++it) {
        if (text[it->offset]) instructions.Add(it->symbol);
    }
}
const char *GetNextWord(const char *input)
{
    while (*input == '\t' || *input == '\n' || *input == ' ') {
        ++currentPosition;
        ++input;
    }
    wordPosition = currentPosition;
    char *dest = word;
    while (*input && *input != ',' && *input != '.' && *input != '[' &&
           *input != ' ' && *input != '\t' && *input != '\n') {
        *dest++ = *input++;
        ++currentPosition;
    }
    *dest = 0;
    return input;
}
