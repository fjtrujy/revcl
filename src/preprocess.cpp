// Translation unit of the preprocessor driver (CIE group 1).

int func_0804ad84(const char *fmt, ...);
char *func_0804a6e0(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

extern const char *matchedDirective;
extern unsigned char toolMask_080b3034;
extern char toolOpt_080b3035;

static inline char lowerCase(char c)
{
    if (c >= 'A' && c <= 'Z')
        c += 'a' - 'A';
    return c;
}

static inline bool startsWith(const char *s, const char *prefix)
{
    const char *p = prefix;
    while (*p && lowerCase(*p) == lowerCase(*s)) {
        s++;
        p++;
    }
    if (*p == 0)
        matchedDirective = prefix;
    return *p == 0;
}

bool func_0804a410(const char *buf)
{
    if (!buf) {
        func_0804ad84("Input was a null pointer?\n");
        return false;
    }
    const char *p = buf;
    bool inComment = false;
    for (; *p; p++) {
        if (*p == ';')
            inComment = true;
        if (*p == '\n')
            inComment = false;
        if (*p == '\r')
            inComment = false;
        if (!inComment) {
            if (startsWith(p, ".mac") || startsWith(p, ".inc")) {
                func_0804ad84("The input seems to include directives that should be processed by gasp, not vcl ('%s...')\n", matchedDirective);
                func_0804ad84("Try adding '-g' to the command line\n");
                return false;
            }
            if (startsWith(p, "#def") || startsWith(p, "#inc")) {
                func_0804ad84("The input seems to include directives that should be processed by gcc, not vcl ('%s..')\n", matchedDirective);
                func_0804ad84("Try adding '-G' to the command line\n");
                return false;
            }
        }
    }
    return true;
}

bool func_0804a608(unsigned char flags)
{
    return (flags & 6) != 0;
}

bool func_0804a61c(unsigned char flags)
{
    return flags & 1;
}

bool func_0804a62c(unsigned char mask, char opt)
{
    if (mask & toolMask_080b3034) {
        if (toolOpt_080b3035 == opt)
            return true;
    }
    return false;
}

void func_0804a658(unsigned char *flags)
{
    *flags |= 1;
}

void func_0804a664(unsigned char *flags)
{
    *flags |= 2;
}

void func_0804a670(unsigned char *flags)
{
    *flags |= 4;
}

char *func_0804a67c(unsigned char flags, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps)
{
    for (int i = 0; i != 3; i++) {
        if ((flags >> i) & 1) {
            buf = func_0804a6e0(i, buf, len, argFirst, argEnd, argv, keepTemps);
            if (!buf)
                return 0;
        }
    }
    return buf;
}
