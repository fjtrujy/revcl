// Byte-exact VUC yyFlexLexer::LexerOutput() provisional source fragment.
// The full generated lexer and its original class declaration are unresolved.
#include <iostream>
#include <cstdlib>
class FlexLexer {
public:
    virtual ~FlexLexer();
};

FlexLexer::~FlexLexer() {}

struct yy_buffer_state {
    void *file;
    char *buffer;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
};

class yyFlexLexer {
public:
    void LexerOutput(const char *text, int length);
    void LexerError(const char *text);
    void yy_switch_to_buffer(yy_buffer_state *next);
    void yy_load_buffer_state();
private:
    char reserved_[0x24];
    std::ostream *output_;
    yy_buffer_state *current_;
    char yy_hold_char_;
    char reserved_align_[3];
    int yy_n_chars_;
    char *yy_c_buf_p_;
    char reserved2_[8];
    int yy_did_buffer_switch_on_eof_;
};

void yyFlexLexer::LexerOutput(const char *text, int length)
{
    output_->write(text, length);
}

void yyFlexLexer::LexerError(const char *text)
{
    std::cerr << text << '\n';
    std::exit(2);
}

void yyFlexLexer::yy_switch_to_buffer(yy_buffer_state *next)
{
    if (current_ == next)
        return;
    if (current_) {
        *yy_c_buf_p_ = yy_hold_char_;
        current_->yy_buf_pos = yy_c_buf_p_;
        current_->yy_n_chars = yy_n_chars_;
    }
    current_ = next;
    yy_load_buffer_state();
    yy_did_buffer_switch_on_eof_ = 1;
}
