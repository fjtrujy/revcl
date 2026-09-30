#include "Program.h"
#include "os.h"

extern bool noAlign_080b3016;

char *emitFileStart(Program *p, char *buf)
{
    if ((!p->proc.unk_00 || p->proc.unk_63) && p->proc.unk_44 < 0)
        buf += func_0804adbc(buf, "\t\t.vu\n");
    if (!noAlign_080b3016)
        buf += func_0804adbc(buf, "\t\t.align 4\n");
    if (p->proc.unk_18 && *p->proc.unk_18) {
        buf += func_0804adbc(buf, "\t\t.global\t%s_CodeStart\n", p->proc.unk_18);
        buf += func_0804adbc(buf, "\t\t.global\t%s_CodeEnd\n", p->proc.unk_18);
    }
    if (p->proc.unk_44 >= 0) {
        if (p->proc.unk_18 && *p->proc.unk_18) {
            buf += func_0804adbc(buf, "\t\t.global\t%s_DmaTag\n", p->proc.unk_18);
            buf += func_0804adbc(buf, "\t\t.global\t%s_DmaEnd\n", p->proc.unk_18);
            buf += func_0804adbc(buf, "\n");
            buf += func_0804adbc(buf, "%s_DmaTag:\n", p->proc.unk_18);
        }
        buf += func_0804adbc(buf, "\t\tDMAret\t*\n");
        buf += func_0804adbc(buf, "\t\tvifnop\t\n");
        buf += func_0804adbc(buf, "\t\tvifnop\t\n");
        buf += func_0804adbc(buf, "\t\tvifnop\t\n");
        buf += func_0804adbc(buf, "\t\tMPG\t%d,*\n", p->proc.unk_44);
    }
    for (mNode *n = p->proc.unk_04.unk_0c; n; n = n->unk_04)
        buf += func_0804adbc(buf, "\t\t.global\t%s\n", n->unk_14);
    if (p->proc.unk_18 && *p->proc.unk_18)
        buf += func_0804adbc(buf, "%s_CodeStart:\n", p->proc.unk_18);
    return buf;
}

char *emitFileEnd(Program *p, char *buf)
{
    if (!noAlign_080b3016)
        buf += func_0804adbc(buf, "\t\t.align 4\n");
    if (p->proc.unk_18 && *p->proc.unk_18)
        buf += func_0804adbc(buf, "%s_CodeEnd:\n", p->proc.unk_18);
    for (mNode *n = p->proc.unk_20.unk_0c; n; n = n->unk_04)
        if (n->unk_14)
            buf += func_0804adbc(buf, "%s:\n", n->unk_14);
    if (p->proc.unk_44 >= 0) {
        buf += func_0804adbc(buf, "\t.EndMPG\n");
        buf += func_0804adbc(buf, "\t.EndDmaData\n");
    }
    if (p->proc.unk_18 && *p->proc.unk_18 && p->proc.unk_44 >= 0)
        buf += func_0804adbc(buf, "%s_DmaEnd:\n", p->proc.unk_18);
    return buf;
}
