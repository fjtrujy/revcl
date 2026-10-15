// Recover genuine old-C++03 ifstream construction, failure-test, EH owner.
// Original VUC 0x0804ca30, length 754. Experimental until exact gate.
#include <fstream>

bool TryOpenFile(const char *path)
{
    std::ifstream input(path);
    if (!input.fail())
        return true;
    return false;
}
