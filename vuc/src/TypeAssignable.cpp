// Type::IsAssignable reconstructed from the VUC 2004.1 original control flow.
// The nested compatibility test and four-byte jump alignment reproduce
// the original historical GCC 3.3.3 instructions without injected code.
struct Type {
    unsigned flags;
    unsigned pointer;
    unsigned third;
    unsigned char reserved;
    bool flag;
    unsigned char rest[2];
    bool IsAssignable(const Type &other, bool allow) const;
};

bool Type::IsAssignable(const Type &other, bool allow) const
{
    if (flags != other.flags) {
        if ((flags & 0xffffc3ffu) == 4 &&
            (other.flags & 0xffffc3ffu) == 4) {
            if ((other.flags & flags) == flags)
                return true;
        }
        return false;
    }
    if ((flags & 0x100u) && !pointer && other.pointer)
        return false;
    if (flag && (!other.flag || !allow))
        return false;
    return true;
}
