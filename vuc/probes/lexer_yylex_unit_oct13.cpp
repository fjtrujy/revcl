// Provisional generated scanner unit: genuine yyFlexLexer scanning APIs.
#include "../src/FlexLexer.cpp"
int yyFlexLexer::yylex()
{
    if (!current_) return 0;
    if (!yy_c_buf_p_ || !*yy_c_buf_p_) {
        yy_load_buffer_state();
        return 0;
    }
    yy_hold_char_ = *yy_c_buf_p_++;
    ++yy_buffer_char_index_;
    return static_cast<unsigned char>(yy_hold_char_);
}
