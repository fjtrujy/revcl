// Byte-exact VUC fragment construction through a nonvirtual IR base.
#include "MLGameString.h"
namespace IR {
class Data {
public:
    int kind;
    Data() : kind(0) {}
};
class Fragment_Data : public Data {
public:
    virtual ~Fragment_Data();
    MLGameString name;
    explicit Fragment_Data(const MLGameString &text) : Data(), name(text) {}
};
}
IR::Fragment_Data *TranslateFrag(const MLGameString &name)
{
    return new IR::Fragment_Data(name);
}
