// Experimental only: reconstructed buffer-load code is not byte exact.
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
    std::istream *file;
    char *buffer;
    char *yy_buf_pos;
    int reserved_0c;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive_internal;
    int yy_fill_buffer;
    int yy_is_interactive;
    int yy_at_bol;
};

class yyFlexLexer {
public:
    void LexerOutput(const char *text, int length);
    void LexerError(const char *text);
    void yy_switch_to_buffer(yy_buffer_state *next);
    void yy_load_buffer_state();
    void yy_flush_buffer(yy_buffer_state *state);
    void yy_init_buffer(yy_buffer_state *state, std::istream *file);
private:
    char reserved0_[0x04];
    char *yytext_ptr_;
    char reserved1_[0x18];
    std::istream *input_;
    std::ostream *output_;
    yy_buffer_state *current_;
    char yy_hold_char_;
    char reserved_align_[3];
    int yy_n_chars_;
    char *yy_c_buf_p_;
    char reserved2_[8];
    int yy_did_buffer_switch_on_eof_;
    char reserved3_[0x28];
    unsigned yy_buffer_char_index_;
};

void yyFlexLexer::yy_load_buffer_state()
{
    yy_n_chars_ = current_->yy_n_chars;
    input_ = current_->file;
    yy_c_buf_p_ = current_->yy_buf_pos;
    yytext_ptr_ = yy_c_buf_p_;
    yy_hold_char_ = *yy_c_buf_p_;
}
