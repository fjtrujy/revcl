// Partial ListFile.cpp source-unit reconstruction retaining genuine method order.
// Generate's file-reading algorithm is not accepted byte-exact. Its presence
// recovers the original global iostream initialization wrapper's register use.
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

class ListFile {
public:
    void Generate(const char *filename);
    int GetNumLines();
    bool PrintLine(int index, std::ofstream &file);
    bool Print(int start, int count, std::ofstream &file);
private:
    std::vector<std::string> lines_;
};

void ListFile::Generate(const char *filename)
{
    std::ifstream input(filename);
    char line[256];
    while (input.getline(line, sizeof(line)))
        lines_.push_back(line);
}

bool ListFile::PrintLine(int line, std::ofstream &output)
{
    if (line <= 0 || line >= GetNumLines()) return false;
    output << "; " << lines_[line - 1].c_str() << std::endl;
    return true;
}

bool ListFile::Print(int start, int count, std::ofstream &file)
{
    int number = GetNumLines();
    if (start < 0) { count -= start; start = 0; }
    int end = start + count;
    if (end > number) end = number;
    for (int i = start; i < end; ++i) PrintLine(i, file);
    return true;
}
int ListFile::GetNumLines() { return lines_.size(); }
