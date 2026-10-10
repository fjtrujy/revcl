// Byte-exact VUC ASTNode arena allocation with original 64-KiB chunks.
// The matching static ASTNode::mem definition is in ASTNodeArenaCleanup.cpp.
#include <new>
#include <cstddef>
class ASTNode {
public:
    static char *mem;
    static void *operator new(std::size_t size);
};
void *ASTNode::operator new(std::size_t size)
{
    if (*(char **)mem + size > mem + 0x10004) {
        char *chunk = (char *)::operator new(0x10008);
        *(char **)chunk = chunk + 4;
        *(char **)(chunk + 0x10004) = mem;
        mem = chunk;
    }
    char *result = *(char **)mem;
    *(char **)mem = result + size;
    return result;
}
