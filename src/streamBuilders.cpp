#include "instStream.h"
#include "flowBlock.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

bool buildPassThru(Program *prog, instStream *s, flowBlock *blocks, int count)
{
    func_0804acb0("BLD_PASS\n");
    if (count != 1) {
        func_0804ad38("cnum isn't one for pass thru case.. it's %d\n", count);
        return false;
    }
    s->unk_124 = blocks->unk_00;
    blocks->unk_00 = 0;
    workTable *uta = &s->unk_148;
    if (!uta->init(1, 0)) {
        func_0804ad38("failed to init uta\n");
        return false;
    }
    uta->unk_10 = s;
    s->unk_2ce = 0;
    s->unk_2d6 = 1;
    s->unk_2d7 = 1;
    return true;
}

bool buildRaw(Program *prog, instStream *s, flowBlock *blocks, int count, int loopMode)
{
    workTable *t = &s->unk_148;
    if (!t) {
        func_0804ad38("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(5, count)) {
        func_0804ad38("failed to init raw table for '%s' with %d entries\n", s->func_0806d674(), count);
        return false;
    }
    t->unk_10 = s;
    if (!t->func_08073774(loopMode)) {
        func_0804ad38("failed to set sub mode\n");
        return false;
    }
    int i = 0;
    int rows = 0;
    bool looped = false;
    Instr *branch = 0;
    int branchRow = -1;
    int branchType = 0;
    for (; i != count; i++) {
        flowBlock *b = &blocks[i];
        if (b->unk_14.func_0804b794()) {
            func_0804ad38("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->unk_14.func_0804b1b0();
        Instr *in = b->unk_0c[0];
        if (in && in->unk_13cd) {
            if (!s->processCmdSynth(in)) {
                func_0804ad84("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                in->func_08054008(false);
                errorOccurred = saved;
            }
            continue;
        }
        wtRow *row = t->func_080724f0(rows);
        if (!row) {
            func_0804ad38("failed to get ied\n");
            return false;
        }
        row->func_080721f8();
        int n = 0;
        int valid = 0;
        for (int j = 0; j != 2; j++) {
            Instr *in = b->unk_0c[j];
            if (!in)
                continue;
            if (in->unk_13cd) {
                func_0804ad38("encountered a synth instruction in raw mode\n");
                return false;
            }
            b->unk_0c[j] = 0;
            if (!t->logAddLinear_Tail(in)) {
                func_0804ad38("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            short lid = in->unk_1420;
            if (lid < 0) {
                func_0804ad38("-ve parseLid set\n");
                return false;
            }
            if (lid) {
                if (in->func_08054138()) {
                    func_0804ad84("loopid set on a nop\n");
                    in->leDump();
                    return false;
                }
                if (!looped && valid) {
                    func_0804ad84("flow::build - hit a looped vsm instruction - but others before it weren't..%d valid \n", valid);
                    in->leDump();
                    return false;
                }
                looped = true;
            } else if (looped && !in->func_08054138()) {
                func_0804ad38("flow::build - parse id wasn't set for a supposedly looped instruction.type\n");
                in->leDump();
                return false;
            }
            unsigned char type = in->unk_1388;
            if (type) {
                if (branchRow != -1) {
                    func_0804ad38("found more than one branch..in what should be continuous sequence\n");
                    return false;
                }
                branchType = type;
                branchRow = rows;
                branch = in;
            }
            row->unk_00[j].unk_00 = in;
            in->setOwner((cls_unk30 *)t);
            valid++;
            n++;
        }
        if (n) {
            if (checkDispatchEntry(&row->unk_00[0].unk_00)) {
                func_0804ad84("invalid instruction combination\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                row->func_0807222c(-1);
                errorOccurred = saved;
                return false;
            }
            rows++;
        }
    }
    s->unk_2ce = branchType;
    if (branchRow >= 0) {
        int d = rows - branchRow - 1;
        if (d <= 0) {
            if (looped) {
                func_0804ad38("in raw_loop mode, vcl expects you to set the branch delay slot!\n");
                branch->leDump();
                return false;
            }
            func_0804ad84("found a raw sequence without a bdl slot instruction.. this could be because mode\n");
            func_0804ad84("flips to UTA , or it could be the bdl slot instruction is also a branch target\n");
            func_0804ad84("for now -this is an error\n");
            return false;
        }
        if (d > 1) {
            func_0804ad38("stray branch instruction found in sequence\n");
            branch->leDump();
            return false;
        }
    }
    if (looped)
        t->setRange(rows);
    else
        t->setRange(0, rows);
    setRangeFrom(t);
    int maxLid = 0;
    for (i = 0; i != rows; i++) {
        wtRow *row = t->func_080724f0(i);
        if (!row) {
            func_0804ad38("failed to get ie from table\n");
            return false;
        }
        for (int j = 0; j != 2; j++) {
            Instr *in = row->unk_00[j].unk_00;
            if (!in)
                continue;
            int lid, pos;
            if (looped) {
                lid = in->unk_1420 - 1;
                pos = i + rows * lid;
            } else {
                pos = i;
                lid = 0;
            }
            in->unk_1420 = -1;
            if (lid > maxLid)
                maxLid = lid;
            in->unk_13e8.func_0805878c(j);
            in->unk_13e8.func_0805874c(pos);
        }
    }
    if (looped && !maxLid) {
        func_0804ad84("max loop id 0 in loop\n");
        return false;
    }
    if (maxLid && !looped) {
        func_0804ad84("max lid not zero and not in a loop\n");
        return false;
    }
    t->unk_3c = maxLid;
    resetRange();
    return true;
}

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
