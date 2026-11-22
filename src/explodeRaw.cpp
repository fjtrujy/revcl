#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *g_080de6ac;
extern mValue *g_080de6b0;
extern int g_080de6b4;
int func_0806aa54(Instr *in);
int expandOneLoopIntoRegions(Program *p, instStream *s);


bool explodeRaw(Program *p)
{
    instStream *next;
    for (instStream *s = p->streamHead; s; s = next) {
        next = s->nextStream;
        workTable *t = s->getTable();
        if (!t) {
            reportError("floBuild - there's no work table!\n");
            return false;
        }
        if (!t->getLoopSize())
            continue;
        if (t->codeModeValue != 3) {
            reportError("explodeRaw: a non raw stream has a loop size set\n");
            return false;
        }
        if (s->knownMinimumLoopCount <= 0) {
            reportWarning("setting known min count to 1 for %s\n", s->streamName());
            s->knownMinimumLoopCount = 1;
        }
        if (s->safeMinimumLoopCount < 0) {
            reportWarning("setting minimum safe count to 1000 for %s\n", s->streamName());
            s->safeMinimumLoopCount = 10000;
        }
        int r = expandOneLoopIntoRegions(p, s);
        if (r < 0) {
            reportError("failed to construct epilogue/prologues\n");
            return false;
        }
        if (r == 0) {
            reportError("no new streams added during build?\n");
            return false;
        }
        delete s;
    }
    return true;
}

extern const char propagateValidMaskError[] = "valid bit mask from new register doesn't contain externally used bits for move %s<-%s\n";
extern const char propagateValidDetailError[] = "valid mask = %x ext bits on output operand = %x\n";
extern const char propagateTwoWritesError[] = "seem to have two writes on same line!\n";
extern const char propagateNormaliseError[] = "failed to normalise instruction\n";
