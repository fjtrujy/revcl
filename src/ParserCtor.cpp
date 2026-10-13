// Parser lifetime and unknown-list cleanup; user-authorized compiler-state reconstruction.

#include "Parser.h"
#include "xpNode.h"
#include "xun.h"
#include "os.h"

int mstrlen(const char *s);
bool func_0804c03c(const char *a, const char *b);
bool func_0804bfcc(const char *a, const char *b);


void func_0804f6ec()
{
    while (g_080bcda4) {
        xun *p = g_080bcda4;
        g_080bcda4 = p->next;
        delete p;
    }
    g_080bcda4 = 0;
    g_080bcda8 = 0;
}

Parser::Parser()
{
    unk_10090 = 0;
    unk_10094 = 0;
    unk_1009c = 0;
    unk_10098 = 0;
    unk_10000 = 0;
}

Parser::~Parser()
{
    reset();
}

void Parser::reset()
{
    if (unk_10090) {
        delete[] unk_10090;
        unk_10090 = 0;
    }
    if (unk_10094) {
        delete[] unk_10094;
        unk_10094 = 0;
    }
    if (unk_1009c) {
        delete[] unk_1009c;
        unk_1009c = 0;
    }
    unk_10098 = 0;
    while (unk_10000) {
        unk_10000--;
        lines[unk_10000].func_0805109c();
    }
}

