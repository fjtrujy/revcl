// Byte-exact original Flex transition logic for the 2004 VUC scanner.
// Other yyFlexLexer members are omitted; accessed offsets are preserved.
extern const unsigned short yy_accept[];
extern const short yy_base[];
extern const short yy_def[];
extern const short yy_chk[];
extern const short yy_nxt[];
extern const int yy_meta[];
extern const int yy_ec[];

class yyFlexLexer {
    char before_yytext_ptr[4];
    char *yytext_ptr;
    char before_buffer_cursor[0x2c];
    char *yy_c_buf_p;
    char before_start[4];
    int yy_start;
    char before_last_accepting[4];
    int yy_last_accepting_state;
    char *yy_last_accepting_cpos;
public:
    int yy_get_previous_state();
};

int yyFlexLexer::yy_get_previous_state()
{
    int yy_current_state = yy_start;
    for (char *yy_cp = yytext_ptr; yy_cp < yy_c_buf_p; ++yy_cp) {
        unsigned char yy_ch = *yy_cp;
        unsigned char yy_c = 1;
        if (yy_ch)
            yy_c = yy_ec[yy_ch];
        if (yy_accept[yy_current_state]) {
            yy_last_accepting_state = yy_current_state;
            yy_last_accepting_cpos = yy_cp;
        }
        while (yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state) {
            yy_current_state = yy_def[yy_current_state];
            if (yy_current_state > 140)
                yy_c = yy_meta[yy_c];
        }
        yy_current_state = yy_nxt[yy_base[yy_current_state] + yy_c];
    }
    return yy_current_state;
}
