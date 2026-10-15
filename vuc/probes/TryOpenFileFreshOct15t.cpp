// First authentic GNU C++03 ifstream TryOpenFile reconstruction.
// Original VUC ELF 0x0804ca30, 754 bytes; no arbitrary instruction source.
#include <fstream>

bool TryOpenFile(const char *name)
{
    std::ifstream input(name);
    return !input.fail();
}
