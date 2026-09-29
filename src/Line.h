// Parser line entry (class name guessed), sizeof 8, no vptr. Defined in group 3.
#ifndef LINE_H
#define LINE_H

#pragma interface

class Line {
public:
    char *unk_00;
    int unk_04;

    Line();
    ~Line();
    bool func_08051054(const char *s, int n);
    void func_0805109c();
    void func_080510cc();

    static const char *typeName(int t)
    {
        const char *s = "LOCTAG_BAD??";
        switch (t) {
        case 0: s = "NULL "; break;
        case 1: s = "FILE "; break;
        case 2: s = "MACRO"; break;
        }
        return s;
    }
};

#endif
