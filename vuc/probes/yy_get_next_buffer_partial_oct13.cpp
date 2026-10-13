// Experimental VUC 2004.1 Flex yy_get_next_buffer() C++03 reconstruction.
// Tables and buffer offsets are recovered from the original ELF; do not
// manufacture binary instructions or string addresses in the source.
#include <cstdio>
#include <cstdlib>
#include <istream>
#include <ostream>

struct yy_buffer_state {
    void *yy_input_file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive;
    int yy_at_bol;
    int yy_fill_buffer;
    int yy_buffer_status;
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
    int yy_get_next_buffer();
private:
    char *yytext_ptr;
    char pad08[0x18];
    std::istream *yyin;
    std::ostream *yyout;
    yy_buffer_state *yy_current_buffer;
    char yy_hold_char;
    char pad2d[3];
    int yy_n_chars;
    char *yy_c_buf_p;
};

int yyFlexLexer::yy_get_next_buffer()
{
    char *dest = yy_current_buffer->yy_ch_buf;
    char *source = yytext_ptr;
    int number_to_move, i;
    int ret_val;
    if (yy_c_buf_p > &yy_current_buffer->yy_ch_buf[yy_n_chars + 1])
        LexerError("fatal flex scanner internal error--end of buffer missed");
    if (yy_current_buffer->yy_fill_buffer == 0) {
        if (yy_c_buf_p - yytext_ptr == 1)
            return 1;
        else
            return 2;
    }
    number_to_move = (int)(yy_c_buf_p - yytext_ptr) - 1;
    for (i = 0; i < number_to_move; ++i)
        *(dest++) = *(source++);
    if (yy_current_buffer->yy_buffer_status == 2)
        yy_current_buffer->yy_n_chars = yy_n_chars = 0;
    else {
        int num_to_read = yy_current_buffer->yy_buf_size - number_to_move - 1;
        while (num_to_read <= 0) {
            yy_buffer_state *b = yy_current_buffer;
            int current_offset = yy_c_buf_p - b->yy_ch_buf;
            if (b->yy_is_our_buffer) {
                char *old_buf = b->yy_ch_buf;
                if (2 * b->yy_buf_size <= 0)
                    b->yy_buf_size += ((unsigned)b->yy_buf_size >> 3);
                else
                    b->yy_buf_size *= 2;
                b->yy_ch_buf = (char*) std::realloc(old_buf, b->yy_buf_size + 2);
            } else
                b->yy_ch_buf = 0;
            if (!b->yy_ch_buf)
                LexerError("fatal error - scanner input buffer overflow");
            yy_c_buf_p = &b->yy_ch_buf[current_offset];
            num_to_read = b->yy_buf_size - number_to_move - 1;
        }
        if (num_to_read > 8192)
            num_to_read = 8192;
        yy_n_chars = LexerInput(&yy_current_buffer->yy_ch_buf[number_to_move], num_to_read);
        if (yy_n_chars < 0)
            LexerError("input in flex scanner failed");
        yy_current_buffer->yy_n_chars = yy_n_chars;
    }
    if (yy_n_chars == 0) {
        if (number_to_move == 0) {
            ret_val = 1;
            yyrestart(yyin);
        } else {
            ret_val = 2;
            yy_current_buffer->yy_buffer_status = 2;
        }
    } else
        ret_val = 0;
    yy_n_chars += number_to_move;
    yy_current_buffer->yy_ch_buf[yy_n_chars] = 0;
    yy_current_buffer->yy_ch_buf[yy_n_chars + 1] = 0;
    yytext_ptr = &yy_current_buffer->yy_ch_buf[0];
    return ret_val;
}
