// Experimental reconstruction of the flex buffer-state reload.
struct yy_buffer_state {
    char *yy_ch_buf;
    unsigned yy_buf_size;
    char *yy_buf_pos;
    char *yy_input_file;
    int yy_n_chars;
};

class yyFlexLexer {
public:
    void yy_load_buffer_state();
private:
    char filler0_[0x4];
    char *yy_c_buf_p;
    char filler1_[0x18];
    char *yytext_ptr;
    char *reserved_state;
    yy_buffer_state *yy_current_buffer;
    char yy_hold_char;
    char filler2_[3];
    int yy_n_chars;
    char *yy_ch_buf;
};

void yyFlexLexer::yy_load_buffer_state()
{
    yy_n_chars = yy_current_buffer->yy_n_chars;
    yytext_ptr = yy_current_buffer->yy_ch_buf;
    yy_ch_buf = yy_current_buffer->yy_buf_pos;
    yy_c_buf_p = yy_ch_buf;
    yy_hold_char = *yy_c_buf_p;
}
