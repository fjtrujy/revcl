// Byte-exact VUC yyFlexLexer::LexerOutput() provisional source fragment.
// The full generated lexer and its original class declaration are unresolved.
#include <ostream>
class yyFlexLexer {
public:
    void LexerOutput(const char *text, int length);
private:
    char reserved_[0x24];
    std::ostream *output_;
};

void yyFlexLexer::LexerOutput(const char *text, int length)
{
    output_->write(text, length);
}
