// Provisional VUC ASTConstValExpr-to-Value parser reconstruction (C++03).
// Offsets grounded in unmodified VUC 2004.1 ELF at 0x0806e7d0.
// The original 455-byte routine copies expression [0xc..0x18] into
// Value[0..0xc] and obtains its MLGameString member at offset 0x1c.
// Diagnostic source only. Do not promote until length, bytes, literals,
// original relocations, and semantics have been independently verified.
#include "../src/MLGameString.h"
#include <cstdlib>
#include <cstring>
// Internal glibc ABI entry points authenticated in the original ELF.
extern "C" long __strtol_internal(const char *, char **, int, int);
extern "C" double __strtod_internal(const char *, char **, int);
struct ASTConstValExpr {
    unsigned char original_ast_header[12];
    int type;
    unsigned qualifier1, qualifier2, properties;
    MLGameString source;
};
struct Value {
    int type;
    unsigned qualifier1, qualifier2, properties;
    union { int bits; float values[4]; };
    void Set(const ASTConstValExpr *);
};
void Value::Set(const ASTConstValExpr *expression)
{
    type = expression->type;
    qualifier1 = expression->qualifier1;
    qualifier2 = expression->qualifier2;
    properties = expression->properties;
    switch (type) {
    case 1:
        bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    case 2: {
        if (std::strcmp(expression->source.c_str(), "true") == 0) bits = 1;
        else if (std::strcmp(expression->source.c_str(), "false") == 0) bits = 0;
        else bits = __strtol_internal(expression->source.c_str(), 0, 10, 0);
        break;
    }
    case 3:
        values[0] = (float)__strtod_internal(expression->source.c_str(), 0, 0);
        break;
    case 4: {
        const char *text = expression->source.c_str();
        char scratch[100];
        for (int i = 0; i < 3; ++i) {
            const char *comma = std::strrchr(text, ',');
            unsigned length = comma - text;
            std::memcpy(scratch, text, length);
            scratch[length] = '\0';
            values[i] = (float)__strtod_internal(scratch, 0, 0);
            text = comma + 1;
        }
        values[3] = (float)__strtod_internal(text, 0, 0);
        break;
    }
    }
}
