#include "Program.h"
#include "os.h"
#include "instStream.h"
#include "VUStr.h"
#include "fileEmit.h"

extern bool noAlign_080b3016;
int getNumOptimisers();
const char *getOptimiserName(int id);

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

char *workTable::emit(char *buf, int mode, int *p1, int *p2)
{
    *p2 = 0;
    *p1 = 0;
    char done = 0;
    int len = unk_34 - unk_30;
    if (len < 0) {
        func_0804ad38("iTable::emit - not done!\n");
        return 0;
    }
    instStream *owner = unk_10;
    if (!owner) {
        func_0804ad38("during emit - owner not set\n");
        return 0;
    }
    int nulls = 0;
    char isEnd = 0;
    switch (owner->unk_2cd) {
    case 2:
        isEnd = 1;
        nulls = 2;
        break;
    case 3:
        isEnd = 1;
        nulls = 2;
        break;
    }
    if (owner->unk_2ce)
        nulls = 2;
    int top = (unsigned char)owner->unk_2d4;

    if (unk_09 != 4) {
        for (int i = unk_30; i != unk_34; i++) {
            wtRow *r = &unk_2c[i];
            if (!r->unk_08)
                (*p1)++;
            (*p2)++;
        }
        if (isEnd && !func_08073854()) {
            (*p2)++;
            (*p1)++;
        }
    } else {
        *p1 = nulls;
        *p2 = nulls;
    }

    if (unk_09 == 4) {
        wtRow blank;
        blank.func_080721f8();
        for (int i = 0; i != nulls; i++) {
            int f = 0;
            if (isEnd && i == 0) {
                f = (unsigned char)(owner->unk_2d5 | 2);
                done = 1;
            }
            f |= top;
            top = 0;
            buf = blank.emit(buf, f, false, false);
            if (!buf) {
                func_0804ad38("failed to emit null ie for synth instruction!\n");
                goto fail;
            }
        }
    } else {
        instStream *s = unk_10;
        if (s) {
            buf += func_0804adbc(buf, "; ");
            if (s->unk_140)
                buf += func_0804adbc(buf, "_LPOPT_");
            else
                buf += func_0804adbc(buf, "_LNOPT_");
            buf += func_0804adbc(buf, "w=[ ");
            if (s->unk_144) {
                for (int i = 0; i != getNumOptimisers(); i++) {
                    int bit = 1 << i;
                    if (s->unk_144 & bit)
                        buf += func_0804adbc(buf, "%s ", getOptimiserName(i));
                }
            }
            buf += func_0804adbc(buf, "] %d [%d %d] %d   [%s]\n",
                                 *p1, unk_74, unk_76, *p2, s->func_0806d674());
        }
        if (len < nulls) {
            func_0804ad38("iTable: not fall. and too small a table\n");
            s->dump(0);
            goto fail;
        }
        *p1 = 0;
        *p2 = 0;
        int end = unk_34;
        if (isEnd && !func_08073854())
            end++;
        wtRow blank;
        blank.func_080721f8();
        for (int i = unk_30; i != end; i++) {
            wtRow *r = &blank;
            if (i < unk_34)
                r = &unk_2c[i];
            int f = 0;
            if (!r->unk_08)
                (*p1)++;
            (*p2)++;
            if (isEnd) {
                if (i == end - 2) {
                    f = (unsigned char)(s->unk_2d5 | 2);
                    done = 1;
                }
            } else if (i == unk_34 - 1)
                f = (unsigned char)s->unk_2d5;
            f |= top;
            top = 0;
            switch (mode) {
            case 0:
                buf = r->emit(buf, f, false, true);
                if (!buf) {
                    func_0804ad38("failed to emit instruction!\n");
                    r->func_0807222c(-1);
                    goto fail;
                }
                break;
            case 1:
                *buf++ = ';';
                buf = r->emit(buf, f, true, true);
                if (!buf) {
                    func_0804ad38("failed to emit instruction!\n");
                    r->func_0807222c(-1);
                    goto fail;
                }
                *buf++ = ' ';
                buf = r->emit(buf, f, false, true);
                if (!buf) {
                    func_0804ad38("failed to emit instruction!\n");
                    r->func_0807222c(-1);
                    goto fail;
                }
                *buf++ = '\n';
                break;
            case 2:
                buf = r->emit(buf, f, true, true);
                if (!buf) {
                    func_0804ad38("failed to emit instruction!\n");
                    r->func_0807222c(-1);
                    goto fail;
                }
                break;
            }
        }
    }
    if (isEnd && !done) {
        func_0804ac7c("did not emit end insn\n");
        return 0;
    }
    return buf;
fail:
    return 0;
}

extern bool noEbit_080b3017;
extern char *stallNames_080b3244[3];
int func_0804bf50(unsigned int);
char *emitNop(char *buf, int flags, int *len);
char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b);

char *wtRow::emit(char *buf, int flags, bool upper, bool unused)
{
    if (!buf) {
        func_0804ad38("iEntry::emit - null pointer!\n");
        return 0;
    }
    if (unk_08) {
        if (func_08072304()) {
            func_0804ad38("can't skip a non-empty ie\n");
            return 0;
        }
        return buf;
    }
    buf += func_0804adbc(buf, "         ");
    int i;
    for (i = 0; i != 2; i++)
        if (unk_00[i].unk_00)
            flags |= unk_00[i].unk_00->unk_1389;
    int len = 0;
    for (i = 0; i != 2; i++) {
        if (!unk_00[i].unk_00)
            buf = emitNop(buf, flags, &len);
        else
            buf = emitInstr(unk_00[i].unk_00, buf, false, flags, upper);
        if (!buf) {
            func_0804ad38("failed to emit upper for pipe %d ie\n", i);
            return 0;
        }
        flags = 0;
        if (i != 1)
            buf += func_0804adbc(buf, "         ");
    }
    if (unk_09) {
        buf += func_0804adbc(buf, "       ;\tSTALL");
        bool any = false;
        for (int k = 0; k != 3; k++) {
            int bit = (unk_0a >> k) & 1;
            if (bit) {
                buf += func_0804adbc(buf, "_%s", stallNames_080b3244[k]);
                any = true;
            }
        }
        if (!any)
            func_0804ac7c("reported a stall.. but dunno what type\n");
        int stall = unk_09;
        buf += func_0804adbc(buf, " ?%d", stall);
    }
    return buf + func_0804adbc(buf, "\n");
}

char *emitNop(char *buf, int flags, int *len)
{
    char line[320];
    char mods[16];
    char *p = line;
    if (noEbit_080b3017)
        flags &= ~2;
    char *s = func_08052314(flags);
    char *d = mods;
    while (*s)
        *d++ = *s++;
    *d = 0;
    p += func_0804adbc(p, "NOP%s", mods);
    for (int n = p - line; n < 50; n++)
        *p++ = ' ';
    *p = 0;
    buf += func_0804adbc(buf, "%s", line);
    return buf;
}

char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b)
{
    char line[320];
    char mods[16];
    char *s = func_08052314(flags);
    char *p = line;
    char *d = mods;
    while (*s)
        *d++ = *s++;
    *d = 0;
    if (in->unk_1392 || !in->unk_34)
        p += func_0804adbc(p, "nop%s", mods);
    else {
        if (in->unk_34->unk_38) {
            p = in->unk_34->unk_38(line, in, upper, mods, b, 0);
            if (!p) {
                func_0804ad38("emit hook failed for an instruction\n");
                return 0;
            }
        } else {
            p = func_0808750c(line, in, upper, mods, b, 0);
            if (!p) {
                func_0804ad38("generic emit hook failed for an instruction\n");
                return 0;
            }
        }
    }
    for (int n = p - line; n < 50; n++)
        *p++ = ' ';
    *p = 0;
    return buf + func_0804adbc(buf, "%s", line);
}

char *emitDestMask(char *p, int mdBits, unsigned char mods)
{
    if (p) {
        switch (mdBits) {
        case 0:
            goto done;
        case 1:
            break;
        case 2:
            if (mods != 7) {
                func_0804ad38("not just one mod bit for MD_XYZ..bits are %x\n", mods);
                return 0;
            }
            break;
        case 3:
            if (func_0804bf50(mods) != 1) {
                func_0804ad38("not just one mod bit for MD_ONE\n");
                return 0;
            }
            break;
        default:
            func_0804ad38("unknown md_bits for vpu target \n");
            return 0;
        }
        if (!mods) {
            func_0804ad38("modBits is null for case MD_ANY.. which should be ok, but ee-dvp-as doesn't assemble it\n");
            return 0;
        }
        if (mods != 15) {
            *p++ = '.';
            if (mods & 1) *p++ = 'x';
            if (mods & 2) *p++ = 'y';
            if (mods & 4) *p++ = 'z';
            if (mods & 8) *p++ = 'w';
        }
    }
done:
    return p;
}
