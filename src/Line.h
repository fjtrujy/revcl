// Source location name and kind (source-file or macro), 8 bytes on i386.
#ifndef LINE_H
#define LINE_H

class SourceLocationEntry {
public:
    char *locationName;
    int locationKind; // 1 = source file, 2 = macro

    SourceLocationEntry();
    ~SourceLocationEntry();
    bool setLocation(const char *name, int kind);
    void clearLocation();
    void dumpLocation();

    static const char *typeName(int kind);
};

#endif
