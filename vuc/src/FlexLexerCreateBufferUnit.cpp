// Reconstructed C++03 FlexLexer buffer-management source unit.
// The original yyrestart predecessor determines GCC 3.3's register state
// for the adjacent yy_create_buffer body.
#include <cstdlib>
#include <istream>
#include <ostream>

struct yy_buffer_state {
    std::istream *yy_input_file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
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
    virtual void yy_switch_to_buffer(yy_buffer_state *);
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    virtual void yy_delete_buffer(yy_buffer_state *);
    virtual void yyrestart(std::istream *);
    virtual int yylex();
    virtual void switch_streams(std::istream *, std::ostream *);
    virtual int LexerInput(char *, int);
    virtual void LexerOutput(const char *, int);
    virtual void LexerError(const char *);
    void yy_init_buffer(yy_buffer_state *, std::istream *);
    void yy_load_buffer_state();
    void yy_flush_buffer(yy_buffer_state *);
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

void yyFlexLexer::yyrestart(std::istream *file)
{
    if (!current_)
        current_ = yy_create_buffer(input_, 0x4000);
    yy_init_buffer(current_, file);
    yy_load_buffer_state();
}

yy_buffer_state *yyFlexLexer::yy_create_buffer(std::istream *file, int size)
{
    yy_buffer_state *b = (yy_buffer_state *)std::malloc(0x28);
    if (!b)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->yy_buf_size = size;
    b->yy_ch_buf = (char *)std::malloc(size + 2);
    if (!b->yy_ch_buf)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->yy_is_our_buffer = 1;
    yy_init_buffer(b, file);
    return b;
}
