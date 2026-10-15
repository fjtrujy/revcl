// First full C++03 reconstruction of original VUC Type -> IR object conversion.
// Keep experimental; the provisional accepted startup TU remains unchanged.
struct Type { unsigned flags, component, width, properties; };
namespace IR {
struct Type_Data {
    virtual void typeInfo();
    unsigned kind;
    explicit Type_Data(unsigned k):kind(k) {}
};
struct UInt_Data : Type_Data {
    UInt_Data():Type_Data(1) {}
};
struct Int_Data : Type_Data {
    Int_Data():Type_Data(2) {}
};
struct Float_Data : Type_Data {
    unsigned width;
    explicit Float_Data(unsigned n):Type_Data(4),width(n) {}
};
struct Float4_Data : Type_Data {
    unsigned channel, width;
    Float4_Data(unsigned c,unsigned w):Type_Data(3),channel(c),width(w) {}
};
struct Array_Data : Type_Data {
    Type_Data *inner;
    unsigned count;
    Array_Data(Type_Data *i,unsigned n):Type_Data(0),inner(i),count(n) {}
};
}

IR::Type_Data *Translate(const Type &type,unsigned options,int allocation)
{
    int kind=type.flags & ~0x3c00u;
    switch (kind) {
        case 1:
          if (!((const unsigned char *)&type)[15])
              return new IR::Int_Data;
          return new IR::UInt_Data;
        case 2:
          return new IR::UInt_Data;
        case 3:
          return new IR::Float_Data(((int)type.width)>>10);
    }
    if ((type.flags & 0x100) && type.component) {
        unsigned width=((int)(type.width | allocation)) >> 10;
        IR::Type_Data *element = new IR::Float4_Data(0,width);
        return new IR::Array_Data(element,type.component);
    }
    if (type.flags & 0x400) {
        unsigned c=(type.flags & options & 0x3c00u) >> 10;
        unsigned width=((int)(type.width | allocation)) >> 10;
        return new IR::Float4_Data(c,width);
    }
    return 0;
}
