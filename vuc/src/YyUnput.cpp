// Byte-exact VUC Flex yyunput C++03 reconstruction of buffer push-back.
#include <istream>
#include <ostream>
struct yy_buffer_state {
    void *yy_input_file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
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
    void yyunput(int, char *);
private:
    char *yytext_ptr;
    char padding08[0x20];
    yy_buffer_state *yy_current_buffer;
    char yy_hold_char;
    char padding2d[3];
    int yy_n_chars;
    char *yy_c_buf_p;
};

void yyFlexLexer::yyunput(int c, char *yy_bp)
{
    char *yy_cp = yy_c_buf_p;
    *yy_cp = yy_hold_char;
    if (yy_cp < yy_current_buffer->yy_ch_buf + 2) {
        int number_to_move = yy_n_chars + 2;
        char *dest = &yy_current_buffer->yy_ch_buf[yy_current_buffer->yy_buf_size + 2];
        char *source = &yy_current_buffer->yy_ch_buf[number_to_move];
        while (source > yy_current_buffer->yy_ch_buf)
            *--dest = *--source;
        yy_cp += (dest - source);
        yy_bp += (dest - source);
        yy_n_chars = yy_current_buffer->yy_buf_size;
        yy_current_buffer->yy_n_chars = yy_n_chars;
        if (yy_cp < yy_current_buffer->yy_ch_buf + 2)
            LexerError("flex scanner push-back overflow");
    }
    *--yy_cp = c;
    yytext_ptr = yy_bp;
    yy_hold_char = *yy_cp;
    yy_c_buf_p = yy_cp;
}
