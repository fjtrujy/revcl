// ExecRP register-pool init handlers, CIE group 3.
#include "Parser.h"
#include "RegPool.h"

RegPool *GetRP(unsigned t);
bool func_080519f8(LocTag *t, RegPool *pool, bool b);

int func_0805116c(Parser *p, LocTag *t)
{
    return func_080519f8(t, GetRP(9), true);
}

int func_0805118c(Parser *p, LocTag *t)
{
    return func_080519f8(t, GetRP(8), true);
}

int func_080511ac(Parser *p, LocTag *t)
{
    return func_080519f8(t, GetRP(9), false);
}

int func_080511cc(Parser *p, LocTag *t)
{
    return func_080519f8(t, GetRP(8), false);
}
