// Byte-exact C++03 reconstruction of the Flex-generated NUL transition.
// Source-level scanner field offsets and original table types are preserved.
// The rest of the yyFlexLexer layout remains provisional.
extern const unsigned short yy_accept[];
extern const short yy_base[];
extern const short yy_def[];
extern const short yy_chk[];
extern const short yy_nxt[];
extern const int yy_meta[];

class yyFlexLexer {
    char before_c_buf_p[0x34];
    char *yy_c_buf_p;
    char before_last_accepting[0x0c];
    int yy_last_accepting_state;
    char *yy_last_accepting_cpos;
public:
    int yy_try_NUL_trans(int yy_current_state);
};

int yyFlexLexer::yy_try_NUL_trans(int yy_current_state)
{
    unsigned char yy_c = 1;
    char *yy_cp = yy_c_buf_p;
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
    return yy_current_state == 140 ? 0 : yy_current_state;
}
