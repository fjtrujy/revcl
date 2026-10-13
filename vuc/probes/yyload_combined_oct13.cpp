#include "../src/FlexLexer.cpp"

// Original load order inferred from GCC 3.3.3 function-level exactness.
void yyFlexLexer::yy_load_buffer_state()
{
    yy_n_chars_ = current_->yy_n_chars;
    yy_c_buf_p_ = current_->yy_buf_pos;
    yytext_ptr_ = yy_c_buf_p_;
    input_ = current_->file;
    yy_hold_char_ = *yy_c_buf_p_;
}
