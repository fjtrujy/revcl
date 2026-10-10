// Experimental flex generated buffer constructor.
#include <cstdlib>
#include <istream>
struct yy_buffer_state {
    std::istream *file;
    char *buffer;
    char *pos;
    int size;
    int count;
    int own;
    int reserved[4];
};
class yyFlexLexer {
public:
    virtual ~yyFlexLexer();
    virtual int yylex();
    virtual yy_buffer_state *yy_create_buffer(std::istream *, int);
    virtual void virtual4();
    virtual void virtual5();
    virtual void virtual6();
    virtual void virtual7();
    virtual void virtual8();
    virtual void virtual9();
    virtual void LexerError(const char *);
    void yy_init_buffer(yy_buffer_state *, std::istream *);
};
yy_buffer_state *yyFlexLexer::yy_create_buffer(std::istream *file, int size)
{
    yy_buffer_state *b = (yy_buffer_state *)std::malloc(0x28);
    if (!b)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->size = size;
    b->buffer = (char *)std::malloc(size + 2);
    if (!b->buffer)
        LexerError("out of dynamic memory in yy_create_buffer()");
    b->own = 1;
    yy_init_buffer(b, file);
    return b;
}
