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

static inline void dumpUtaInstruction(Instr *in)
{
    in->func_08054008(false);
}


bool buildUta(Program *prog, instStream *s, DecodedLine *blocks, int count)
{
    workTable *t = &s->unk_148;
    if (!t) {
        reportError("build: failed to get input table from ist\n");
        return false;
    }
    if (!t->init(1, 0)) {
        reportError("failed to init uta table for '%s'\n", s->streamName());
        return false;
    }
    t->unk_10 = s;
    int flags = 0;
    int branch = -1;
    for (int i = 0; i != count; i++) {
        DecodedLine *b = &blocks[i];
        if (b->labels.func_0804b794()) {
            reportError("started to build the stream.. but there was a label decorated with debug info in the way!\n");
            return false;
        }
        b->labels.func_0804b1b0();
        Instr *in = b->instructions[0];
        if (!in)
            continue;
        if (in->unk_1420 > 0) {
            reportUserMessage("parseLid set for a uta instruction\n");
            in->leDump();
            return false;
        }
        in->unk_1420 = -1;
        if (in->unk_13cd) {
            if (!s->processCmdSynth(in)) {
                reportUserMessage("failed to process an instruction\n");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                dumpUtaInstruction(in);
                errorOccurred = saved;
            }
            flags |= in->unk_1389;
        } else {
            b->instructions[0] = 0;
            if (!t->logAddLinear_Tail(in)) {
                reportError("failed to attach instruction to the linear list for synth\n");
                return false;
            }
            if (in->unk_1388) {
                if (branch >= 0) {
                    reportError("more than one branch in uta decode\n");
                    return false;
                }
                branch = in->unk_1388;
            }
        }
    }
    if (branch < 0)
        branch = 0;
    s->unk_2ce = branch;
    if (flags)
        reportInfo("d_viFlags - is set to %x on exit<-----------------------------\n", flags);
    return true;
}

int func_0806e530(int flags);
void func_0806ed78(Program *p, int a);
bool fixupBtorBranch(Program *p, instStream *s, int *changed);
