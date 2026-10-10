// Experimental VUC yy_flush_buffer inferred from original Flex state.
#include <istream>
struct yy_buffer_state {
    std::istream *file;
    char *buffer;
    char *yy_buf_pos;
    int reserved_0c;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive_internal;
    int yy_fill_buffer;
    int yy_is_interactive;
    int yy_at_bol;
};
class yyFlexLexer {
public:
    void yy_flush_buffer(yy_buffer_state *state);
    void yy_load_buffer_state();
private:
    char reserved_[0x28];
    yy_buffer_state *current_;
    char reserved2_[0x40];
    unsigned yy_buffer_char_index_;
};
void yyFlexLexer::yy_flush_buffer(yy_buffer_state *state)
{
    if (!state) return;
    state->yy_n_chars = 0;
    state->buffer[yy_buffer_char_index_] = 0;
    state->buffer[yy_buffer_char_index_ + 1] = 0;
    state->yy_buf_pos = state->buffer;
    state->yy_fill_buffer = 1;
    state->yy_at_bol = 0;
    if (state == current_)
        yy_load_buffer_state();
}
