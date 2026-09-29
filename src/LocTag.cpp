// lineEntry (LocTag) methods, CIE group 3.
#include "Parser.h"
#include "os.h"

char *LocTag::func_0805086c(int *mode, char *in)
{
    if (!unk_00) {
        func_0804ad38("lineEntry::Tokenise() - m_l_LineData was null\n");
        return 0;
    }
    if (!in) {
        func_0804ad38("lineEntry::Tokenise() - in_p was null\n");
        return 0;
    }
    if (*mode != 0 && *mode != 2) {
        func_0804ad38("invalid comment mode at start of lineEntry::Tokenise\n");
        return 0;
    }
    char *p = in;
    int state = 1;
    unk_8c = 0;
    int n = 0;
    TokenEntry *te = 0;
    TokenEntry *last = 0;
    char *out = unk_00;
    while (*p) {
        bool split = false;
        bool isTok = func_08050708(*p);
        if (*mode == 2) {
            if (n > 1 && p[-2] == '*' && p[-1] == '/') {
                *mode = 0;
                split = true;
            }
        } else if (*p == '/' && p[1] == '*') {
            *mode = 2;
            split = true;
        } else if (*p == '/' && p[1] == '/') {
            if (*mode != 3)
                split = true;
            *mode = 3;
        } else if (*p == ';') {
            if (*mode != 1)
                split = true;
            *mode = 1;
        }
        if (*mode == 0) {
            if (n > 1 && *p != ':' && p[-1] == ':' && p[-2] != ':')
                split = true;
            if (*p == 0 || (n > 0 && p[-1] == 0))
                split = true;
        }
        if (*mode != 0)
            isTok = true;
        if (split && te) {
            te->unk_02 = 0;
            *out++ = 0;
            state = 1;
            te = 0;
        }
        switch (state) {
        case 1:
            if (te) {
                func_0804ad38("tokeniser had unexpected non-null te\n");
                return 0;
            }
            if (isTok) {
                if (unk_8c >= 200) {
                    func_0804ad38("lineEntry::Tokenise: too many tokens on line\n");
                    return 0;
                }
                te = &unk_090[unk_8c];
                te->unk_00 = out - unk_00;
                te->unk_02 = 0;
                te->unk_03 = *mode != 0;
                last = te;
                *out++ = *p;
                state = 0;
                unk_8c++;
            } else if (*p == ',') {
                if (!last) {
                    func_0804acb0("unexpected comma encountered\n");
                    return 0;
                }
                last->unk_02 = 1;
                last = 0;
            }
            break;
        case 0:
            if (!isTok) {
                if (*p == ',') {
                    if (!last) {
                        func_0804acb0("unexpected comma encountered\n");
                        return 0;
                    }
                    last->unk_02 = 1;
                    last = 0;
                }
                *out++ = 0;
                state = 1;
                te = 0;
            } else {
                if (!te) {
                    func_0804ad38("tokeniser had unexpected null TE\n");
                    return 0;
                }
                *out++ = *p;
            }
            break;
        }
        n++;
        p++;
    }
    if (te) {
        te->unk_02 = 2;
        *out++ = 0;
    }
    if (n > 1 && p[-2] == '*' && p[-1] == '/')
        *mode = 0;
    if (*mode != 2)
        *mode = 0;
    if (!unk_8c)
        unk_00 = 0;
    return out;
}

char *LocTag::func_08050be4(int i, char *type)
{
    if (!unk_8c)
        return 0;
    if (i < 0 || i >= unk_8c)
        return 0;
    if (!unk_00) {
        func_0804ad38("lineEntry: get token - but m_lineData was null\n");
        return 0;
    }
    if (type)
        *type = unk_090[i].unk_02;
    return unk_00 + unk_090[i].unk_00;
}
