// Reconstruct real lexer buffer-load behavior in the recovered lexer source unit.
#include "../src/FlexLexer.cpp"
void yyFlexLexer::yy_load_buffer_state()
{
    yy_buffer_state *buf = current_;
    int count = buf->yy_n_chars;
    std::istream *file = buf->file;
    yy_n_chars_ = count;
    char *pos = buf->yy_buf_pos;
    input_ = file;
    yy_c_buf_p_ = pos;
    yytext_ptr_ = pos;
    yy_hold_char_ = *pos;
}
