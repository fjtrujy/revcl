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
    virtual ~yyFlexLexer();
    virtual int yylex();
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    void LexerOutput(const char *text, int length);
    void LexerError(const char *text);
    void yy_switch_to_buffer(yy_buffer_state *next);
    void yy_load_buffer_state();
    void yy_flush_buffer(yy_buffer_state *state);
    void yy_init_buffer(yy_buffer_state *state, std::istream *file);
    void yy_delete_buffer(yy_buffer_state *state);
private:
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

void yyFlexLexer::yy_init_buffer(yy_buffer_state *state, std::istream *file)
{
    yy_flush_buffer(state);
    state->yy_is_interactive = 1;
    state->yy_is_interactive_internal = 0;
    state->file = file;
}

void yyFlexLexer::yy_flush_buffer(yy_buffer_state *state)
{
    if (!state) return;
    state->yy_n_chars = 0;
    state->buffer[yy_buffer_char_index_] = 0;
    state->buffer[yy_buffer_char_index_ + 1] = 0;
    state->yy_buf_pos = state->buffer;
    state->yy_fill_buffer = 1;
    state->yy_at_bol = 0;
    if (state == current_)
        yy_load_buffer_state();
}

void yyFlexLexer::yy_delete_buffer(yy_buffer_state *state)
{
    if (!state) return;
    if (state == current_) current_ = 0;
    if (state->yy_is_our_buffer) std::free(state->buffer);
    std::free(state);
}
