#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

static inline void dumpSynthInstruction(Instr *in)
{
    in->func_08054008(false);
}


bool buildSynth(Program *prog, instStream *s, DecodedLine *blocks, int count)
{
    workTable *t = &s->unk_148;
    if (!t) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(4, 0)) {
        reportError("failed to init synth table for '%s'\n", s->streamName());
        return false;
    }
    t->unk_10 = s;
    int inSynth;
    int stype;
    int flags = 0;
    inSynth = 0;
    stype = 0;
    for (int i = 0; i != count; i++) {
        DecodedLine *b = &blocks[i];
        if (b->labels.combinedFlags()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->labels.clearNodes();
        Instr *in = b->instructions[0];
        if (!in)
            continue;
        bool isIO = false;
        if (in->unk_1420 > 0) {
            reportUserMessage("parseLid set for a synth instruction\n");
            in->leDump();
            return false;
        }
        in->unk_1420 = -1;
        switch ((unsigned char)in->unk_13cd) {
        case 11:
            if (s->unk_2d0) {
                reportUserMessage("istream already has end macro attached\n");
                return false;
            }
            if (!in->operands[0].unk_1c.getString()) {
                reportError("EXITM has no macro string attached\n");
                return false;
            }
            s->unk_2d0 = dupString(in->operands[0].unk_1c.getString());
        case 1:
        case 3:
            if (stype) {
                const char *a = func_080554e4((unsigned char)in->unk_13cd);
                reportError("flow related synthetic (%s) encountered.. but stype not null (%s)!\n", a, func_080554e4(stype));
                in->leDump();
                return false;
            }
            inSynth = stype = (unsigned char)in->unk_13cd;
            break;
        case 2:
            if (stype != 1) {
                reportError("unexpected --endenter encountered\n");
                in->leDump();
                return false;
            }
            if (!inSynth) {
                reportError("flow related synthetic encountered.. but the block didn't appear to be wholly synthetic\n");
                in->leDump();
                return false;
            }
            inSynth = 0;
            break;
        case 4:
            if (!inSynth) {
                reportError("end exit encountered, but not in synthetic mode\n");
                in->leDump();
                return false;
            }
            if (stype != 3 && stype != 11) {
                reportError("unexpected --endexit encountered\n");
                in->leDump();
                return false;
            }
            inSynth = 0;
            break;
        case 6:
            isIO = true;
            if (stype != 1) {
                reportUserMessage("synthetic input not in enter block\n");
                in->leDump();
                return false;
            }
            break;
        case 7:
            if (stype != 3 && stype != 11) {
                reportUserMessage("synthetic output not in exit block\n");
                in->leDump();
                return false;
            }
            isIO = true;
            break;
        default:
            reportUserMessage("encountered an misplaced instruction in a enter or exit block\n");
            dumpSynthInstruction(in);
            return false;
        }
        if (isIO) {
            b->instructions[0] = 0;
            if (!t->logAddLinear_Tail(in)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
        } else
            flags |= in->unk_1389;
    }
    switch (stype) {
    case 11:
        s->unk_2cd = 4;
        if (!s->unk_2d0) {
            reportError("exitmacro not set.. but end_macro endtype?\n");
            return false;
        }
        break;
    case 3:
        s->unk_2cd = 3;
        break;
    case 1:
        s->unk_2cc = 2;
        break;
    default:
        reportUserMessage("vcl found a problem making an enter/exit block, and I'll improve this error message later\n");
        return false;
    }
    if (flags)
        reportInfo("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
}
