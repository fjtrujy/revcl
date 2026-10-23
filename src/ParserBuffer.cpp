// Parser initialization and processed-buffer construction.

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


bool __attribute__((weak,section(".gnu.linkonce.t._ZN6Parser13func_0804fb94EPcPKcbb"))) Parser::func_0804fb94(char *buf, const char *name, bool gasp, bool cpp)
{
    reset();
    if (!buf) {
        func_0804ad38("Parser::Init: passed a null pointer to buffer\n");
        reset();
        return false;
    }
    int len = mstrlen(buf);
    if (!len) {
        func_0804ad38("Parser:: was passed a null string!\n");
        reset();
        return false;
    }
    unk_10090 = new char[len + 16];
    if (!unk_10090) {
        func_0804ad38("Parser:: malloc failed for raw buffer\n");
        reset();
        return false;
    }
    mstrcpy(unk_10090, buf);
    if (func_0805030c(name, 1)) {
        func_0804ad38("Parser::failed to add initial loctag!\n");
        return false;
    }
    unk_1008c = 1;
    unk_1000c[0].unk_04 = 0;
    unk_1000c[0].unk_00 = lines;
    if (!func_0804fd80(gasp, cpp)) {
        func_0804ad38("Parser: failed to build proc buffer\n");
        reset();
        return false;
    }
    return true;
}

char *g_080b3144 = "0123456789.";    // gasp line-number chars
char *g_080b3148 = "\"'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890.:{}<>()[]+-*/\\$#@!~%^&_=|?;";    // token (non-whitespace) char set
char *g_080b314c = "----*";    // block-start keyword
char *g_080b3150 = "*----";    // block-end keyword

bool func_08050780(const char *s, const char *kw);

static inline bool isEol(char c)
{
    return c == '\n' || c == '\r';
}

static inline bool isNewline(char c)
{
    return c == '\n';
}

static inline void dumpLineEntry(LocTag *entry, int highlight)
{
    entry->func_08050c40(highlight);
}

bool Parser::func_0804fd80(bool gasp, bool cpp)
{
    if (!unk_10090) {
        func_0804ad38("Parser: build proc buffer - raw buffer was null pointer\n");
        reset();
        return false;
    }
    int len = mstrlen(unk_10090);
    if (!len) {
        func_0804ad38("arser: raw buffer was null string\n");
        reset();
        return false;
    }
    unk_10094 = new char[len * 2];
    if (!unk_10094) {
        func_0804ad38("Parser: malloc failed for proc buffer\n");
        reset();
        return false;
    }
    int nlines = 0;
    char ch;
    for (char *p = unk_10090; (ch = *p); p++)
        if (isNewline(ch))
            nlines++;
    unk_1009c = new LocTag[nlines];
    if (!unk_1009c) {
        func_0804ad38("Parser: failed to malloc line table\n");
        reset();
        return false;
    }
    unk_10098 = 0;
    LocTag *t;
    int lineNo = 0;
    char *next;
    char *line = unk_10090;
    int commentMode = 0;
    char *out = unk_10094;
    char head = *line;
    for (; head; line = next, head = *line) {
        char *p = line;
        if (!head || head == '\n') {
            head = *line;
        } else {
            do {
                p++;
                head = *p;
            } while (head && head != '\n');
        }
        char *eol = p;
        while (head && isEol(head)) {
            p++;
            head = *p;
        }
        next = p;
        *eol = 0;
        bool process = true;
        if (*line == ';' && gasp) {
            if (!func_0805041c(line)) {
                func_0804ad38("failed to decode a gasp line\n");
                return false;
            }
            process = false;
        }
        int checkBlock = process;
        if ((char)checkBlock && func_08050780(line, g_080b314c)) {
            char *q = next;
            t = &unk_1009c[unk_10098++];
            t->unk_04 = lineNo;
            char rawHead = *next;
            for (;;) {
                if (!rawHead)
                    break;
                if (func_08050780(q, g_080b3150)) {
                    rawHead = *q;
                    break;
                }
                rawHead = *q;
                while (rawHead && !isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                while (rawHead && isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                lineNo++;
            }
            char *e = q;
            if (rawHead) {
                while (rawHead && !isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                while (rawHead && isEol(rawHead)) {
                    q++;
                    rawHead = *q;
                }
                lineNo++;
            }
            *e = 0;
            t->unk_0a = -1;
            t->unk_00 = next;
            t->unk_08 = 1;
            next = q;
            process = false;
        }
        if (process) {
            char *s = line;
            char c;
            while ((c = *s) && !({
                const char *q = g_080b3148;
                char tok;
                while ((tok = *q) && tok != c)
                    q++;
                tok != 0;
            }))
                s++;
            t = &unk_1009c[unk_10098];
            if (gasp) {
                t->unk_0a = unk_1008c;
                for (int i = 0; i != unk_1008c; i++)
                    t->unk_0c[i] = unk_1000c[i];
            } else {
                t->unk_0a = -1;
            }
            t->unk_04 = lineNo;
            unk_10098++;
            t->unk_00 = out;
            out = t->func_0805086c(&commentMode, line);
            if (!out) {
                func_0804ad38("Parser: failed to tokenise line '%s'\n", line);
                dumpLineEntry(t, -1);
                return false;
            }
        }
        lineNo++;
    }
    return true;
}

// Shared diagnostics retain their aligned pool across the parser split.
extern const char parserDumpNoTable[] = "Parse::Dump() - line table was not set\n";
extern const char parserLocTagLimit[] __attribute__((aligned(32))) = "AddLocTag: ran out of loctags\n";
extern const char parserGaspIncludeTagFailure[] = "failed to add loc tag for include directive\n";
extern const char parserGaspMissingPreviousTag[] = "neither previous include or previous macro set\n";
extern const char parserGaspDepthJump[] __attribute__((aligned(32))) = "jumped too far  in gasp depth\n";
extern const char parserGaspDepthOverflow[] = "exceeded maximum depth for the loctag stack\n";
