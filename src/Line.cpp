#include "Line.h"
#include "os.h"

char *dupString(const char *s);

static inline const char *lineTypeNameInl(int kind)
{
    const char *name = "LOCTAG_BAD??";
    switch (kind) {
    case VCL_SOURCE_LOCATION_NONE: name = "NULL "; break;
    case VCL_SOURCE_LOCATION_FILE: name = "FILE "; break;
    case VCL_SOURCE_LOCATION_MACRO: name = "MACRO"; break;
    }
    return name;
}

SourceLocationEntry::SourceLocationEntry()
{
    locationName = 0;
    locationKind = VCL_SOURCE_LOCATION_NONE;
}

SourceLocationEntry::~SourceLocationEntry()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = VCL_SOURCE_LOCATION_NONE;
}

bool SourceLocationEntry::setLocation(const char *name, int kind)
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = VCL_SOURCE_LOCATION_NONE;
    locationName = dupString(name);
    locationKind = (VclSourceLocationKind)kind;
    return true;
}

void SourceLocationEntry::clearLocation()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = VCL_SOURCE_LOCATION_NONE;
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
