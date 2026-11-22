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

Line::Line()
{
    locationName = 0;
    locationKind = 0;
}

Line::~Line()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
}

bool Line::setLocation(const char *name, int kind)
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
    locationName = dupString(name);
    locationKind = kind;
    return true;
}

void Line::clearLocation()
{
    if (locationName)
        delete[] locationName;
    locationName = 0;
    locationKind = 0;
}

void Line::dumpLocation()
{
    if (locationName) {
        int kind = locationKind;
        reportMessage("[%s : '%s']", lineTypeNameInl(kind), locationName, kind);
    } else
        reportMessage("[EMPTY_LOCTAG]");
}

const char *Line::typeName(int kind)
{
    return lineTypeNameInl(kind);
}
