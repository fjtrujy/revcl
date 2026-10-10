// Evidence-driven type-layout hypotheses for small VUC functions.
// These are experimental reconstructions, not authenticated upstream headers.
#include <vector>

struct MLGameStringData {
    unsigned reference_count;
    const char *text;
};

class MLGameString {
public:
    const char *c_str() const;
private:
    MLGameStringData *data_;
};

const char *MLGameString::c_str() const
{
    return data_->text;
}

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
