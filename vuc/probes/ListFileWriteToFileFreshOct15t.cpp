// Real old-C++03 ListFile::WriteToFile source-level reconstruction.
// Original VUC function at 0x0805abc0 has 1071 bytes and uses the
// authenticated ListFile 12-byte std::vector<std::string> layout.
// Keep probe-only until original size, relocations and literals authenticate.
#include <fstream>
#include <vector>
#include <string>
class ListFile {
public:
    bool WriteToFile(const char *path);
private:
    std::vector<std::string> lines_;
};
bool ListFile::WriteToFile(const char *path)
{
    std::ofstream output(path);
    if (!output) return false;
    int n=lines_.size();
    for (int i=0;i<n;++i) {
        output << i << ":\t" << lines_[i].c_str();
        output << std::endl;
    }
    return true;
}
