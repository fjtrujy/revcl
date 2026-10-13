// Partial original Translate.cpp source-unit reconstruction for the VUC IR.
#include <iostream>
#include "../src/MLGameString.h"
struct Type {
    unsigned flags, component, width, properties;
};
namespace IR {
class Data { public: int kind; Data(): kind(0) {} virtual ~Data(); };
struct Value_Data : Data { unsigned value; explicit Value_Data(unsigned v): value(v) {kind=1;} };
struct Register_Data : Data { unsigned reg, channel; Register_Data(unsigned r,unsigned c):reg(r),channel(c){kind=3;} };
}
IR::Data *Translate(const Type &type, unsigned options, int allocation)
{
    unsigned kind=type.flags & ~0x3c00u;
    if (kind == 1 && !type.properties) return new IR::Value_Data(0);
    if (kind == 2) return new IR::Value_Data(1);
    if (kind == 3) return new IR::Register_Data(type.width >> 10, 0);
    if ((type.flags & 0x100u) && type.component)
        return new IR::Register_Data(type.width >> 10, options & 0x3c00u);
    return 0;
}
