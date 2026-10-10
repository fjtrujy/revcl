// Byte-exact VUC Instruction arena allocation with original 8-KiB chunks.
// The original static Instruction::instrData is defined here.
#include <new>
#include <cstddef>
class Instruction {
public:
    static char *instrData;
    static void *operator new(std::size_t size);
};
char *Instruction::instrData;
void *Instruction::operator new(std::size_t size)
{
    if (*(char **)instrData + size > instrData + 0x2004) {
        char *chunk = (char *)::operator new(0x2008);
        *(char **)chunk = chunk + 4;
        *(char **)(chunk + 0x2004) = instrData;
        instrData = chunk;
    }
    char *result = *(char **)instrData;
    *(char **)instrData = result + size;
    return result;
}
