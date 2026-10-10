// VUC compiler fingerprint probes: use idioms visible in the original ELF.
// Compile only; there is intentionally no main or dependency on REVCL.
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

struct ProbeRecord {
    virtual ~ProbeRecord();
    virtual int score(int) const;
    int count;
};

ProbeRecord::~ProbeRecord() {}
int ProbeRecord::score(int value) const { return value + count; }

static const char *kBanner = "VUC-2004-probe";

void probe_emit(std::ofstream &out)
{
    out << kBanner;
}

int probe_vector(std::vector<int> &values, int needle)
{
    for (std::vector<int>::const_iterator it = values.begin(); it != values.end(); ++it) {
        if (*it == needle)
            return static_cast<int>(it - values.begin());
    }
    return -1;
}

int probe_exception(int value)
{
    if (value < 0)
        throw std::runtime_error("negative value");
    return value + 1;
}

static ProbeRecord global_record;

ProbeRecord *probe_global()
{
    return &global_record;
}
