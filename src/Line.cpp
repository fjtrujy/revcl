// Parser location-tag methods; user-authorized separate compilation.
#pragma implementation "Line.h"
#include "Line.h"
#include "os.h"

char *dupString(const char *s);

Line::Line()
{
    unk_00 = 0;
    unk_04 = 0;
}

Line::~Line()
{
    if (unk_00)
        delete[] unk_00;
    unk_00 = 0;
    unk_04 = 0;
}

bool Line::func_08051054(const char *s, int n)
{
    if (unk_00)
        delete[] unk_00;
    unk_00 = 0;
    unk_04 = 0;
    unk_00 = dupString(s);
    unk_04 = n;
    return true;
}

void Line::func_0805109c()
{
    if (unk_00)
        delete[] unk_00;
    unk_00 = 0;
    unk_04 = 0;
}

void Line::func_080510cc()
{
    if (unk_00) {
        int t = unk_04;
        func_0804ac50("[%s : '%s']", typeName(t), unk_00, t);
    } else
        func_0804ac50("[EMPTY_LOCTAG]");
}
