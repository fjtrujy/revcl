// Original-style VUC monotonic arena allocation hypothesis.
#include <new>
#include <cstddef>
struct ASTNode {
    static char *mem;
    static void *operator new(unsigned n);
};
struct Instruction {
    static char *instrData;
    static void *operator new(unsigned n);
};
char *ASTNode::mem;
char *Instruction::instrData;

template <unsigned Capacity>
static inline __attribute__((always_inline)) void *arena_allocate(char *&chunk, unsigned n)
{
    char *at = *(char **)chunk;
    if (at + n > chunk + 4 + Capacity) {
        char *fresh = (char *)::operator new(Capacity + 8);
        *(char **)fresh = fresh + 4;
        *(char **)(fresh + 4 + Capacity) = chunk;
        chunk = fresh;
        at = *(char **)chunk;
    }
    *(char **)chunk = at + n;
    return at;
}

void *ASTNode::operator new(unsigned n)
{
    return arena_allocate<0x10000>(mem, n);
}
void *Instruction::operator new(unsigned n)
{
    return arena_allocate<0x2000>(instrData, n);
}
