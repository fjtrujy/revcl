// Reconstructed VUC ListFile.cpp fragment.
// Only ListFile::GetNumLines() is verified byte-exact against VUC 2004.1.
// Other class declarations and translation-unit content remain incomplete.
#include <vector>
#include <fstream>
#include <string>

class ListFile {
public:
    int GetNumLines();
    bool PrintLine(int index, std::ofstream &file);
    bool Print(int start, int count, std::ofstream &file);
private:
    std::vector<std::string> lines_;
};

int ListFile::GetNumLines()
{
    return lines_.size();
}

bool ListFile::Print(int start, int count, std::ofstream &file)
{
    int number = GetNumLines();
    if (start < 0) {
        count -= start;
        start = 0;
    }
    int end = start + count;
    if (end > number) end = number;
    for (int i = start; i < end; ++i)
        PrintLine(i, file);
    return true;
}
