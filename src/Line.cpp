#include "Line.h"
#include "os.h"

char *dupString(const char *s);

static inline const char *lineTypeNameInl(int kind)
{
    const char *name = "LOCTAG_BAD??";
    switch (kind) {
    case 0: name = "NULL "; break;
    case 1: name = "FILE "; break;
    case 2: name = "MACRO"; break;
    }
    return name;
}

SourceLocationEntry::SourceLocationEntry()
{
    locationName = 0;
    locationKind = 0;
}

SourceLocationEntry::~SourceLocationEntry()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
}

bool SourceLocationEntry::setLocation(const char *name, int kind)
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
    locationName = dupString(name);
    locationKind = kind;
    return true;
}

void SourceLocationEntry::clearLocation()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
}

void SourceLocationEntry::dumpLocation()
{
    if (locationName) {
        int kind = locationKind;
        reportMessage("[%s : '%s']", lineTypeNameInl(kind), locationName, kind);
    } else
        reportMessage("[EMPTY_LOCTAG]");
}

const char *SourceLocationEntry::typeName(int kind)
{
    return lineTypeNameInl(kind);
}
