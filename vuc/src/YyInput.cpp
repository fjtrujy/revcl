// Byte-exact Flex C++ yyinput buffer and EOF handling for VUC 2004.1.
// The original uses the case-2 -> case-1 EOF fallthrough after yyrestart.
#include <istream>
struct yy_buffer_state {
    void *yy_input_file;
    char *yy_ch_buf;
    int yy_buf_pos;
    int yy_buf_size;
};
class yyFlexLexer {
public:
    virtual ~yyFlexLexer();
    virtual void yy_switch_to_buffer(yy_buffer_state *);
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    virtual void yy_delete_buffer(yy_buffer_state *);
    virtual void yyrestart(std::istream *);
    int yyinput();
    int yy_get_next_buffer();
private:
    char *yytext_ptr;
    char padding08[0x18];
    std::istream *yyin;
    std::ostream *yyout;
    yy_buffer_state *yy_current_buffer;
    char yy_hold_char;
    char padding2d[3];
    int yy_n_chars;
    char *yy_c_buf_p;
};
int yyFlexLexer::yyinput()
{
    int c;
    *yy_c_buf_p = yy_hold_char;
    if (*yy_c_buf_p == 0) {
        if (yy_c_buf_p < &yy_current_buffer->yy_ch_buf[yy_n_chars]) {
            *yy_c_buf_p = 0;
        } else {
            int offset = yy_c_buf_p - yytext_ptr;
            ++yy_c_buf_p;
            switch (yy_get_next_buffer()) {
            case 2:
                yyrestart(yyin);
            case 1:
                return -1;
            case 0:
                yy_c_buf_p = yytext_ptr + offset;
                break;
            }
        }
    }
    c = (unsigned char)*yy_c_buf_p;
    *yy_c_buf_p = 0;
    yy_hold_char = *++yy_c_buf_p;
    return c;
}
