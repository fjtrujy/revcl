// ExecRP register-pool init handlers, CIE group 3.
#include "Parser.h"
#include "RegPool.h"
#include "Program.h"
#include "os.h"

char *dupString(const char *s);
extern bool noAlign_080b3016;

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

inline bool ExecRP_init_reg_all(RegPool *pool)
{
    if (!pool) {
        func_0804ad38("ExecRP_init_reg_all: passed a null pool\n");
        return false;
    }
    return pool->func_08059c00();
}

int func_080511ec(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(9));
}

int func_08051228(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(8));
}

int func_08051264(Program *prog, LocTag *t)
{
    noAlign_080b3016 = true;
    func_0804acb0("dh_align_disable() - disabling .align emit\n");
    return 1;
}

int func_08051284(Program *prog, LocTag *t)
{
    noAlign_080b3016 = true;
    func_0804acb0("dh_no_ebits() - disabling [e]bit emit\n");
    return 1;
}

int func_080512a4(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!s) {
        func_0804ad38("failed to get global name\n");
        return 0;
    } else {
        prog->unk_18 = dupString(s);
    }
    return 1;
}

int func_08051308(Program *prog, LocTag *t)
{
    prog->unk_63 = true;
    return 1;
}

int func_0805131c(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!s) {
        func_0804ad38("must specify target address for mpg mode!\n");
        return 0;
    }
    prog->unk_44 = func_0804add4(s);
    return 1;
}

int func_0805137c(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (s)
        prog->unk_04.func_0804b2d4(s);
    return 1;
}
