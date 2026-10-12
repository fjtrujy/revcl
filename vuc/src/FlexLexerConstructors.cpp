// Byte-exact original-generated yyFlexLexer constructors (C1 and C2).
// The full class declaration remains provisional, but original
// field offsets and the original vtable relocation are verified.
#include <istream>
#include <ostream>
struct yy_buffer_state;
class yyFlexLexer {
public:
    virtual ~yyFlexLexer();
    yyFlexLexer(std::istream *input, std::ostream *output);
private:
    unsigned char unused_04[8];
    int start_state_;       // +0x0c
    int offset_10_;         // +0x10
    int offset_14_;         // +0x14
    int offset_18_;         // +0x18
    int offset_1c_;         // +0x1c
    std::istream *input_;   // +0x20
    std::ostream *output_;  // +0x24
    yy_buffer_state *buffer_; // +0x28
    int unused_2c_[2];     // +0x2c and +0x30
    int offset_34_;
    int offset_38_;
    int offset_3c_;
    int offset_40_;
    unsigned char reserved_44[8];
    int offset_4c_;
    unsigned char reserved_50[20];
    int offset_64_;
    int offset_68_;
    int offset_6c_;
    int offset_70_;
    int offset_74_;
};
yyFlexLexer::yyFlexLexer(std::istream *input, std::ostream *output)
{
    input_ = input;
    offset_34_ = 0;
    offset_38_ = 1;
    output_ = output;
    offset_3c_ = 0;
    offset_10_ = 0;
    start_state_ = 1;
    offset_40_ = 0;
    offset_64_ = 0;
    offset_68_ = 0;
    offset_6c_ = 0;
    offset_74_ = 0;
    offset_70_ = 0;
    offset_18_ = 0;
    offset_14_ = 0;
    offset_1c_ = 0;
    buffer_ = 0;
    offset_4c_ = 0;
}
