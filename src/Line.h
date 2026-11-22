// Parser line entry (class name guessed), sizeof 8, no vptr. Defined in group 3.
#ifndef LINE_H
#define LINE_H

class Line {
public:
    char *locationName;
    int locationKind; // 1 = source file, 2 = macro

    Line();
    ~Line();
    bool setLocation(const char *name, int kind);
    void clearLocation();
    void dumpLocation();

    static const char *typeName(int kind);
};

#endif
