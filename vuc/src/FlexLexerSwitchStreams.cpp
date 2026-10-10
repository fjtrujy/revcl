// VUC scanner stream transition; vtable ordering from original indirect calls.
#include <istream>
#include <ostream>
struct yy_buffer_state;
class yyFlexLexer {
public:
    virtual ~yyFlexLexer();
    virtual void yy_switch_to_buffer(yy_buffer_state *);
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    virtual void yy_delete_buffer(yy_buffer_state *);
    void switch_streams(std::istream *, std::ostream *);
private:
    char pad_[0x20];
    std::ostream *output_;
    yy_buffer_state *current_;
};
void yyFlexLexer::switch_streams(std::istream *input, std::ostream *output)
{
    if (input) {
        yy_delete_buffer(current_);
        yy_switch_to_buffer(yy_create_buffer(input, 0x4000));
    }
    if (output) output_ = output;
}
