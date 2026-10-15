// Experimental original Type stream input reconstruction, C++03 style.
// Original 369-byte function: 264 matching bytes, 10 relocations,
// 48 verified literal bytes. Its 16-byte Type local lifetime remains partial.
#include <istream>
#include <cstring>
#include <cstdlib>

struct Type {
    unsigned kind;
    unsigned array_length;
    unsigned reserve;
    unsigned char parsed, is_const, zero_array, auxiliary;
    Type() : kind(0), array_length(0), reserve(0),
             parsed(0), is_const(0), zero_array(0), auxiliary(0) {}
};

extern "C" long __strtol_internal(const char *, char **, int, int);

std::istream &operator>>(std::istream &input, Type &type)
{
    static const char *tn[] = {
        "error", "int", "bool", "float", "float4", "function", "void"
    };
    Type temporary;
    char text[1024];
    type = temporary;
    input >> text;
    if (std::strcmp(text, "const") == 0) {
        type.is_const = 1;
        input >> text;
    }
    for (unsigned i = 0; i <= 6; ++i)
        if (std::strstr(text, tn[i]))
            type.kind = i;
    if (type.kind == 4)
        type.kind = 0x3c04;
    char *left = std::strchr(text, '[');
    if (left) {
        type.kind |= 0x100;
        char *right = std::strchr(text, ']');
        *right = 0;
        type.array_length = __strtol_internal(left + 1, 0, 10, 0);
        if (type.array_length == 0)
            type.zero_array = 1;
    }
    type.parsed = 1;
    if (std::strstr(text, "const"))
        type.is_const = 0;
    type.reserve = 0;
    return input;
}
