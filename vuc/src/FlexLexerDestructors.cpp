// Byte-exact generated FlexLexer destructor behavior and ABI.
#include <new>
struct yy_buffer_state;
class FlexLexer {
public:
    virtual ~FlexLexer() {}
};
class yyFlexLexer : public FlexLexer {
public:
    virtual ~yyFlexLexer();
    void yy_delete_buffer(yy_buffer_state *);
private:
    char pad0_[0x24];
    yy_buffer_state *current_;
    char pad1_[0x20];
    char *yy_state_buf_;
};
yyFlexLexer::~yyFlexLexer()
{
    ::operator delete(yy_state_buf_);
    yy_delete_buffer(current_);
}
