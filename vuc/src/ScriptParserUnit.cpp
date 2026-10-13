// VUC original-era partial source-unit reconstruction.
// Only the accepted startup callback, not the large application body, is byte-exact.
// VUC generated parser source-unit hypothesis from original yacc/Bison APIs.
// The full parse automaton and original parse tables are not reconstructed.
#include <iostream>
#include <vector>
int script_debug;
extern int script_lex();
extern void script_error(const char *);
struct ASTNode { void apa(); };
int script_parse()
{
    std::vector<int> parserStates;
    parserStates.push_back(0);
    int token;
    while ((token = script_lex()) != 0) {
        if (script_debug) std::cerr << "parser token: " << token << '\n';
        switch (token) {
        case 1:
        case 2:
            parserStates.push_back(token);
            break;
        default:
            if (parserStates.empty()) {
                script_error("syntax error");
                return 1;
            }
            parserStates.pop_back();
            break;
        }
    }
    return parserStates.empty() ? 1 : 0;
}
void ASTNode::apa() {}
