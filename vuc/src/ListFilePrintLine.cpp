// Byte-exact VUC ListFile::PrintLine and inputFileName startup fragment.
// This provisional source-unit grouping is supported by original wrapper bytes.
#include <iostream>
char inputFileName[256] = {};
#include <vector>
#include <string>
#include <fstream>
class ListFile {
public:
    int GetNumLines();
    bool PrintLine(int line, std::ofstream &output);
private:
    std::vector<std::string> lines_;
};
bool ListFile::PrintLine(int line, std::ofstream &output)
{
    if (line <= 0 || line >= GetNumLines()) return false;
    output << "; " << lines_[line - 1].c_str() << std::endl;
    return true;
}
