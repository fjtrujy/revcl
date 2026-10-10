// Experimental original yyFlexLexer::yy_delete_buffer layout and ownership.
#include <cstdlib>
struct yy_buffer_state {
    void *file;
    char *yy_ch_buf;
    char *yy_buf_pos;
    int yy_buf_size;
    int yy_n_chars;
    int yy_is_our_buffer;
};
class yyFlexLexer {
public:
    void yy_delete_buffer(yy_buffer_state *buffer);
private:
    char pad_[0x28];
    yy_buffer_state *yy_current_buffer;
};
void yyFlexLexer::yy_delete_buffer(yy_buffer_state *buffer)
{
    if (!buffer) return;
    if (buffer == yy_current_buffer)
        yy_current_buffer = 0;
    if (buffer->yy_is_our_buffer)
        std::free(buffer->yy_ch_buf);
    std::free(buffer);
}
