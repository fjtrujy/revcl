#include "instStream.h"
#include "flowBlock.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);


bool buildSynth(Program *prog, instStream *s, flowBlock *blocks, int count)
{
    workTable *t = &s->unk_148;
    if (!t) {
        func_0804ad38("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(4, 0)) {
        func_0804ad38("failed to init synth table for '%s'\n", s->func_0806d674());
        return false;
    }
    t->unk_10 = s;
    int inSynth = 0;
    int stype = 0;
    int flags = 0;
    for (int i = 0; i != count; i++) {
        flowBlock *b = &blocks[i];
        if (b->unk_14.func_0804b794()) {
            func_0804ad38("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->unk_14.func_0804b1b0();
        Instr *in = b->unk_0c[0];
        if (!in)
            continue;
        bool isIO = false;
        if (in->unk_1420 > 0) {
            func_0804ad84("parseLid set for a synth instruction\n");
            in->leDump();
            return false;
        }
        in->unk_1420 = -1;
        switch ((unsigned char)in->unk_13cd) {
        case 11:
            if (s->unk_2d0) {
                func_0804ad84("istream already has end macro attached\n");
                return false;
            }
            if (!in->unk_38[0].unk_1c.getString()) {
                func_0804ad38("EXITM has no macro string attached\n");
                return false;
            }
            s->unk_2d0 = dupString(in->unk_38[0].unk_1c.getString());
        case 1:
        case 3:
            if (stype) {
                const char *a = func_080554e4((unsigned char)in->unk_13cd);
                const char *c = func_080554e4(stype);
                func_0804ad38("flow related synthetic (%s) encountered.. but stype not null (%s)!\n", a, c);
                in->leDump();
                return false;
            }
            inSynth = stype = (unsigned char)in->unk_13cd;
            break;
        case 2:
            if (stype != 1) {
                func_0804ad38("unexpected --endenter encountered\n");
                in->leDump();
                return false;
            }
            if (!inSynth) {
                func_0804ad38("flow related synthetic encountered.. but the block didn't appear to be wholly synthetic\n");
                in->leDump();
                return false;
            }
            inSynth = 0;
            break;
        case 4:
            if (!inSynth) {
                func_0804ad38("end exit encountered, but not in synthetic mode\n");
                in->leDump();
                return false;
            }
            if (stype != 3 && stype != 11) {
                func_0804ad38("unexpected --endexit encountered\n");
                in->leDump();
                return false;
            }
            inSynth = 0;
            break;
        case 6:
            isIO = true;
            if (stype != 1) {
                func_0804ad84("synthetic input not in enter block\n");
                in->leDump();
                return false;
            }
            break;
        case 7:
            if (stype != 3 && stype != 11) {
                func_0804ad84("synthetic output not in exit block\n");
                in->leDump();
                return false;
            }
            isIO = true;
            break;
        default:
            func_0804ad84("encountered an misplaced instruction in a enter or exit block\n");
            in->func_08054008(false);
            goto fail;
        }
        if (isIO) {
            b->unk_0c[0] = 0;
            if (!t->logAddLinear_Tail(in)) {
                func_0804ad38("failed to attach instruction to the linear list for synth\n");
                return false;
            }
        } else
            flags |= in->unk_1389;
    }
    switch (stype) {
    case 1:
        s->unk_2cc = 2;
        break;
    case 3:
        s->unk_2cd = 3;
        break;
    case 11:
        s->unk_2cd = 4;
        if (!s->unk_2d0) {
            func_0804ad38("exitmacro not set.. but end_macro endtype?\n");
            return false;
        }
        break;
    default:
        func_0804ad84("vcl found a problem making an enter/exit block, and I'll improve this error message later\n");
        return false;
    }
    if (flags)
        func_0804acb0("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
fail:
    return false;
}
