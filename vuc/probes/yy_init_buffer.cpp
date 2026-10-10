// Reconstruct VUC yyFlexLexer::yy_init_buffer() for original buffer layout.
#include <istream>
struct yy_buffer_state {
    std::istream *yy_input_file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive_internal;
    int yy_is_interactive_extra;
    int yy_is_interactive;
};
class yyFlexLexer {
public:
    void yy_flush_buffer(yy_buffer_state *state);
    void yy_init_buffer(yy_buffer_state *state, std::istream *file);
};

void yyFlexLexer::yy_init_buffer(yy_buffer_state *state, std::istream *file)
{
    yy_flush_buffer(state);
    state->yy_is_interactive = 1;
    state->yy_is_interactive_internal = 0;
    state->yy_input_file = file;
}
