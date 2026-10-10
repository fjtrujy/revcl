// Byte-exact reconstruction of Flex generated yyrestart() behavior.
#include <istream>
struct yy_buffer_state;
class yyFlexLexer {
public:
    virtual ~yyFlexLexer();
    virtual int yylex();
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    void yyrestart(std::istream *file);
    void yy_init_buffer(yy_buffer_state *, std::istream *);
    void yy_load_buffer_state();
private:
    char padding0_[0x1c];
    std::istream *input_;
    char padding1_[0x04];
    yy_buffer_state *current_;
};
void yyFlexLexer::yyrestart(std::istream *file)
{
    if (!current_)
        current_ = yy_create_buffer(input_, 0x4000);
    yy_init_buffer(current_, file);
    yy_load_buffer_state();
}
