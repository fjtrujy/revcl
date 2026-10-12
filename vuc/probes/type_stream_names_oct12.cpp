// Exploratory C++ Type stream syntax: source-generated per-function name tables.
// The class/streaming bodies remain unverified; only semantic type-name array
// provenance is currently being investigated.
#include <iostream>
#include <string>
struct Type {
    unsigned int kind;
    unsigned int value;
    unsigned int flags;
    unsigned int extra;
};
std::istream &operator>>(std::istream &input, Type &value)
{
    static const char *tn[] = {"error", "int", "bool", "float", "float4", "function", "void"};
    std::string name;
    input >> name;
    for (unsigned i = 0; i < sizeof(tn) / sizeof(tn[0]); ++i) {
        if (name == tn[i]) {
            value.kind = i;
            break;
        }
    }
    return input;
}
std::ostream &operator<<(std::ostream &output, Type type)
{
    static const char *tn[] = {"error", "int", "bool", "float", "float4", "function", "void"};
    return output << tn[type.kind % (sizeof(tn) / sizeof(tn[0]))];
}
