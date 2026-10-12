// Byte-exact generated FlexLexer destructor behavior and ABI.
#include <new>
#include <iosfwd>
struct yy_buffer_state;
class FlexLexer {
public:
    virtual ~FlexLexer() {}
    virtual void yy_switch_to_buffer(yy_buffer_state *) = 0;
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int) = 0;
    virtual void yy_delete_buffer(yy_buffer_state *) = 0;
    virtual void yyrestart(std::istream *) = 0;
    virtual int yylex() = 0;
    virtual void switch_streams(std::istream *, std::ostream *) = 0;
};
class yyFlexLexer : public FlexLexer {
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
