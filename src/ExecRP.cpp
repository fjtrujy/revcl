// ExecRP register-pool init handlers, CIE group 3.
#include "Parser.h"
#include "RegPool.h"
#include "Program.h"
#include "os.h"

char *dupString(const char *s);
extern bool noAlign_080b3016;
extern int errorOccurred;
extern bool quiet_080bbd98;

bool func_0804bfcc(const char *a, const char *b);
RegPool *GetRP(unsigned t);
bool func_080519f8(LocTag *t, RegPool *pool, bool b);
bool func_08051d6c(Program *prog, LocTag *t, int v);
bool func_08051e34(Program *prog, LocTag *t, int v);
bool func_08088f20(RegPool *pool, const char *name, int size);
int mstrlen(const char *s);

bool func_0805116c(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(9), true);
}

bool func_0805118c(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(8), true);
}

bool func_080511ac(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(9), false);
}

bool func_080511cc(Program *prog, LocTag *t)
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

bool func_080511ec(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(9));
}

bool func_08051228(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(8));
}

bool func_08051264(Program *prog, LocTag *t)
{
    noAlign_080b3016 = true;
    func_0804acb0("dh_align_disable() - disabling .align emit\n");
    return 1;
}

bool func_08051284(Program *prog, LocTag *t)
{
    noAlign_080b3016 = true;
    func_0804acb0("dh_no_ebits() - disabling [e]bit emit\n");
    return 1;
}

bool func_080512a4(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!s) {
        func_0804ad38("failed to get global name\n");
        return 0;
    } else {
        prog->proc.unk_18 = dupString(s);
    }
    return 1;
}

bool func_08051308(Program *prog, LocTag *t)
{
    prog->proc.unk_63 = true;
    return 1;
}

bool func_0805131c(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!s) {
        func_0804ad38("must specify target address for mpg mode!\n");
        return 0;
    }
    prog->proc.unk_44 = func_0804add4(s);
    return 1;
}

bool func_0805137c(Program *prog, LocTag *t)
{
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (s)
        prog->proc.unk_04.func_0804b2d4(s);
    return 1;
}

bool func_080513cc(Program *prog, LocTag *t)
{
    if (prog->proc.unk_3f) {
        func_0804ad84("hit a raw directive in a rawloop block\n");
        return 0;
    }
    if (prog->proc.unk_3d == 3)
        func_0804ac7c("hit a raw directive while already in a raw block\n");
    prog->proc.unk_3d = 3;
    prog->proc.unk_3e = 4;
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (s) {
        if (func_0804bfcc(s, "reorder"))
            prog->proc.unk_3e = 3;
        else if (func_0804bfcc(s, "noreorder"))
            prog->proc.unk_3e = 4;
        else {
            func_0804ad84("bad option '%s' to raw directive\n", s);
            return 0;
        }
    }
    return 1;
}

bool func_080514a0(Program *prog, LocTag *t)
{
    if (prog->proc.unk_3d == 1) {
        func_0804ac7c("hit an endraw directive while not in raw mode\n");
        return 0;
    }
    if (prog->proc.unk_3f) {
        func_0804ad84("hit an endraw directive inside a rawloop\n");
        return 0;
    }
    prog->proc.unk_3d = 1;
    prog->proc.unk_3e = 0;
    return 1;
}

bool func_080514e8(Program *prog, LocTag *t)
{
    if (prog->proc.unk_3f) {
        func_0804ad84("encountered a .rawloop directive, while already in loop mode\n");
        return 0;
    }
    prog->proc.unk_40++;
    prog->proc.unk_62 = prog->proc.unk_3d;
    prog->proc.unk_3d = 3;
    prog->proc.unk_3e = 4;
    prog->proc.unk_3f = true;
    char *s = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (s) {
        if (func_0804bfcc(s, "reorder"))
            prog->proc.unk_3e = 3;
        else if (func_0804bfcc(s, "noreorder"))
            prog->proc.unk_3e = 4;
        else {
            func_0804ad84("bad option '%s' to raw loop directive\n", s);
            return 0;
        }
    }
    return 1;
}

bool func_080515a0(Program *prog, LocTag *t)
{
    if (!prog->proc.unk_3f) {
        func_0804ad84("reached an .endloop directive but when wasn't in loop mode\n");
        return 0;
    }
    if (prog->proc.unk_3d != 3) {
        func_0804ad84("hit end raw directive, but wasn't in VSM mode..(mode is %d)?\n", prog->proc.unk_3d);
        return 0;
    }
    prog->proc.unk_3d = prog->proc.unk_62;
    prog->proc.unk_3e = prog->proc.unk_61;
    prog->proc.unk_3f = false;
    return 1;
}

bool func_080515f0(Program *prog, LocTag *t)
{
    int device = -1;
    int limit = -1;
    char *cpu = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!cpu) {
        func_0804ad38("failed to get token cpu (1) from .work directive\n");
        return 0;
    }
    if (func_0804bfcc(cpu, "core")) {
        func_0804ad38("work: core cpu work area.. not yet\n");
        return 0;
    }
    if (func_0804bfcc(cpu, "vu0")) {
        device = 1;
        limit = 0x100;
    }
    if (func_0804bfcc(cpu, "vu1")) {
        device = 2;
        limit = 0x400;
    }
    if (limit < 0) {
        func_0804ad38("work: target device not set defining a work area.. must be one of core,vu0 or vu1!\n");
        return 0;
    }
    char *ea = t->func_08050f18(2) ? 0 : t->func_08050be4(2, 0);
    if (!ea) {
        func_0804ad38("failed to get effective address token (2) from .work directive\n");
        return 0;
    }
    char *sz = t->func_08050f18(3) ? 0 : t->func_08050be4(3, 0);
    if (!sz) {
        func_0804ad38("failed to get size token (3) from .work directive\n");
        return 0;
    }
    int addr = func_0804add4(ea);
    int size = func_0804add4(sz);
    if (size <= 0) {
        func_0804ad38("size is <=0 for a work area\n");
        return 0;
    }
    if (addr < 0) {
        func_0804ad38("ea is negative for a work area\n");
        return 0;
    }
    if (addr + size > limit) {
        func_0804ad38("work area overruns size of device\n");
        return 0;
    }
    RegPool *pool = GetRP(11);
    if (!pool) {
        func_0804ad38("init work area - failed to get vu mem pool\n");
        return 0;
    }
    if (!func_08088f20(pool, ea, size)) {
        func_0804ad38("failed to init the mem area!\n");
        return 0;
    }
    func_0804ac7c("work_area: created work area for device %d - [%4d:%-4d]\n", device, addr, size);
    return 1;
}

inline bool ExecRP_skip_directive(LocTag *t)
{
    func_0804ac7c("skipping directive:\n");
    t->func_08050c40(-1);
    return true;
}

bool func_08051808(Program *prog, LocTag *t)
{
    return ExecRP_skip_directive(t);
}

bool func_08051830(Program *prog, LocTag *t)
{
    return func_08051d6c(prog, t, 0);
}

bool func_08051848(Program *prog, LocTag *t)
{
    return func_08051d6c(prog, t, 1);
}

bool func_08051860(Program *prog, LocTag *t)
{
    return func_08051e34(prog, t, 0);
}

bool func_08051878(Program *prog, LocTag *t)
{
    return func_08051e34(prog, t, 1);
}

bool func_08051890(Program *prog, LocTag *t)
{
    char *name = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (name) {
        if (mstrlen(name) > 128) {
            func_0804ad84("unique name too long\n");
            return 0;
        }
        prog->proc.setUniqueName(name);
    }
    prog->proc.unk_60 = 1;
    return 1;
}

bool func_08051908(Program *prog, LocTag *t)
{
    char *arg = t->func_08050f18(1) ? 0 : t->func_08050be4(1, 0);
    if (!arg) {
        func_0804ad84("must specify an argument to '.syntax', valid ones are 'new' or 'old'\n");
        return 0;
    }
    if (func_0804bfcc(arg, "new")) {
        prog->proc.unk_00 = true;
        return 1;
    }
    if (func_0804bfcc(arg, "old")) {
        prog->proc.unk_00 = false;
        return 1;
    }
    func_0804ad84("invalid argument to .syntax, options are 'new' or 'old'\n");
    return 0;
}

Directive directives_080b3160[28] = {
    {".vu", func_08051308},
    {".mpg", func_0805131c},
    {".vsm", func_080513cc},
    {".endvsm", func_080514a0},
    {".vsmloop", func_080514e8},
    {".endvsmloop", func_080515a0},
    {".init_vf", func_0805116c},
    {".init_vi", func_0805118c},
    {".rem_vf", func_080511ac},
    {".rem_vi", func_080511cc},
    {".init_vf_all", func_080511ec},
    {".init_vi_all", func_08051228},
    {".vu_work_area", func_080515f0},
    {".name", func_080512a4},
    {".syntax", func_08051908},
    {".align", func_08051808},
    {".global", func_0805137c},
    {".noaligns", func_08051264},
    {".noebits", func_08051284},
    {".raw", func_080513cc},
    {".endraw", func_080514a0},
    {".rawloop", func_080514e8},
    {".endrawloop", func_080515a0},
    {".unique", func_08051890},
    {".show", func_08051848},
    {".hide", func_08051830},
    {".endshow", func_08051878},
    {".endhide", func_08051860},
};

inline char *ExecRP_directive_name(LocTag *t)
{
    return t->func_08050be4(0, 0);
}

Directive *func_080519a0(LocTag *t)
{
    if (!t)
        return 0;
    char *name = ExecRP_directive_name(t);
    Directive *d = directives_080b3160;
    for (int i = 0; i != 28; i++, d++)
        if (func_0804bfcc(name, d->name))
            return d;
    return 0;
}

bool func_080519f8(LocTag *t, RegPool *pool, bool flag)
{
    char buf[2][0x800];
    int i = 1;
    if (!pool) {
        func_0804ad38("ExecRP_init_reg: passed a null pool\n");
        return false;
    }
    for (; i < func_0805085c(t); i++) {
        char *tok = t->func_08050f18(i) ? 0 : t->func_08050be4(i, 0);
        if (!tok)
            continue;
        int range = 0;
        char *d = buf[0];
        for (char *s = tok; *s; s++) {
            if (*s == '-') {
                if (range) {
                    func_0804ad84("too many '-' chars in %s\n", tok);
                    return false;
                }
                range = 1;
                *d = 0;
                d = buf[1];
            } else
                *d++ = *s;
        }
        *d = 0;
        int first = -1, last;
        int j;
        for (j = 0; j <= range; j++) {
            RegPoolEntry *r = pool->func_08059418(buf[j]);
            if (!r) {
                func_0804ad38("not find %s\n", tok);
                return false;
            }
            if (!r->unk_15) {
                func_0804ad38("init_reg: %s is not allocable\n", r->func_0805a8e0());
                return false;
            }
            if (r->unk_14) {
                func_0804ad38("init_reg: reg is virtual! %s\n", r->func_0805a8e0());
                return false;
            }
            if (j == 0)
                first = last = r->unk_30;
            else
                last = r->unk_30;
        }
        if (last < first) {
            func_0804ac7c("range %s decreasing\n", tok);
            int tmp = last;
            last = first;
            first = tmp;
        }
        for (j = first; j <= last; j++) {
            RegPoolEntry *r = pool->func_080593e0(j);
            if (!r) {
                func_0804ad38("not find %s\n", tok);
                return false;
            }
            if (!r->unk_15) {
                func_0804ad38("init_reg: %s is not allocable\n", r->func_0805a8e0());
                return false;
            }
            if (r->unk_14) {
                func_0804ad38("init_reg: reg is virtual! %s\n", r->func_0805a8e0());
                return false;
            }
            r->unk_16 = flag;
        }
    }
    return true;
}

int func_08051c30(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(1), true);
}

int func_08051c50(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(2), true);
}

int func_08051c70(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(1), false);
}

int func_08051c90(Program *prog, LocTag *t)
{
    return func_080519f8(t, GetRP(2), false);
}

int func_08051cb0(LocTag *t, RegPool *pool)
{
    if (!pool) {
        func_0804ad38("ExecRP_init_reg_all: passed a null pool\n");
        return 0;
    }
    return pool->func_08059c00();
}

int func_08051ce0(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(1));
}

int func_08051d1c(Program *prog, LocTag *t)
{
    return ExecRP_init_reg_all(GetRP(2));
}

int func_08051d58(Program *prog, LocTag *t)
{
    return 1;
}

int func_08051d64(Program *prog, LocTag *t)
{
    return 0;
}

bool func_08051d6c(Program *prog, LocTag *t, int v)
{
    char buf[0x800];
    int left = 0x7ff;
    char *d = buf;
    for (int i = 1; i < func_0805085c(t); i++) {
        char *tok = t->func_08050f18(i) ? 0 : t->func_08050be4(i, 0);
        if (!tok)
            continue;
        while (*tok) {
            *d++ = *tok++;
            if (--left == 0) {
                func_0804ad38("ran out of local space for visible list\n");
                return false;
            }
        }
        *d++ = ':';
    }
    *d = 0;
    mNode *n = prog->proc.unk_4c.func_0804b444(buf);
    if (!n) {
        func_0804ad38("failed to process  a visibility directive\n");
        return false;
    }
    n->unk_18 = v;
    return true;
}

inline void ExecRP_show_line(LocTag *t)
{
    t->func_08050c40(-1);
}

bool func_08051e34(Program *prog, LocTag *t, int v)
{
    mNode *n = prog->proc.unk_4c.unk_10;
    if (!n) {
        func_0804ac7c("closing visibility directive and no tail at:\n");
        int saved = errorOccurred;
        errorOccurred = !quiet_080bbd98;
        ExecRP_show_line(t);
        errorOccurred = saved;
        return true;
    }
    if (n->unk_18 != v) {
        func_0804ad84("mode mismatch closing a visibility scope\n");
        int saved = errorOccurred;
        errorOccurred = !quiet_080bbd98;
        ExecRP_show_line(t);
        errorOccurred = saved;
        return false;
    }
    if (!prog->proc.unk_4c.func_0804b3e8(n)) {
        func_0804ad38("failed to remove tail node\n");
        return false;
    }
    delete n;
    return true;
}
