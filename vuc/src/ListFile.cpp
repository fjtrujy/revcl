// Reconstructed VUC ListFile.cpp fragment.
// Only ListFile::GetNumLines() is verified byte-exact against VUC 2004.1.
// Other class declarations and translation-unit content remain incomplete.
#include <vector>

class ListFile {
public:
    int GetNumLines();
private:
    std::vector<const char *> lines_;
};

int ListFile::GetNumLines()
{
    return lines_.size();
}
