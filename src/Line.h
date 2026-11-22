// Source location name and kind (source-file or macro), 8 bytes on i386.
#ifndef LINE_H
#define LINE_H

// Selects an existing diagnostic label; the label text stays in Line.cpp.
enum VclSourceLocationKind {
    VCL_SOURCE_LOCATION_NONE = 0,
    VCL_SOURCE_LOCATION_FILE = 1,
    VCL_SOURCE_LOCATION_MACRO = 2
};

class SourceLocationEntry {
public:
    char *locationName;
    VclSourceLocationKind locationKind;

    SourceLocationEntry();
    ~SourceLocationEntry();
    bool setLocation(const char *name, int kind);
    void clearLocation();
    void dumpLocation();

    static const char *typeName(int kind);
};

#endif
