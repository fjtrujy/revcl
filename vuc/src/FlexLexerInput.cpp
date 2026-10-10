// Experimental reconstruction of historical FlexLexer C++ istream input.
#include <istream>
class yyFlexLexer {
public:
    int LexerInput(char *buffer, int maxSize);
private:
    char padding_[0x20];
    std::istream *input_;
};
int yyFlexLexer::LexerInput(char *buffer, int maxSize)
{
    if (input_->eof() || input_->fail()) return 0;
    input_->get(buffer[0]);
    if (input_->eof()) return 0;
    return input_->bad() ? -1 : 1;
}
