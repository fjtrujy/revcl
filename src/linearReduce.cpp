#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, InstructionPosition *pos);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
void func_0806ed78(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int func_0807dc44(workTable *wt, Instr *i, InstructionPosition *pos);
bool setRange(int start, int end, int lsz);
int pullUp(workTable *wt);
int dragDown(workTable *wt);

static inline bool isPipeMode(workTable *wt)
{
    int mode = wt->unk_09 - 3;
    return !mode && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

static inline void dumpLinearReduce(Program *p)
{
    func_0806ed78(p, 0);
}

bool func_0807e740(Program *p, workTable *wt, unsigned char up, unsigned char down)
{
    if (!isPipeMode(wt))
        return true;
    if (wt->getLoopSize())
        return true;

    int passes = up != 0;
    if (down)
        passes++;

    wt->getRangeStart();
    wt->getRangeEnd();
    setRangeFrom(wt);

    int count = passes;
    while (count) {
        count--;
        if (up) {
            int r = pullUp(wt);
            if (r < 0) {
                reportError("failed to pull up\n");
                dumpLinearReduce(p);
                return false;
            }
            if (r)
                count = passes;
        }
        if (down) {
            int r = dragDown(wt);
            if (r < 0) {
                reportError("failed drag down\n");
                return false;
            }
            if (r)
                count = passes;
        }
    }
    return true;
}

bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl)
{
    InstructionPosition pos;
    bool found = false;
    bool upwards;

    if (dl == -1) {
        if (s_test < e_test) {
            reportError("do_localMove - s_test<e_test\n");
            return false;
        }
        upwards = true;
    } else if (dl == 1) {
        if (s_test > e_test) {
            reportError("do_localMove + s_test>e_test\n");
            return false;
        }
        upwards = false;
    } else {
        reportError("bad dl for doLocalMove\t .. %d\n", dl);
        return false;
    }

    for (; s_test != e_test; s_test += dl) {
        pos.setFromLinearRow(s_test);
        if (!func_0807e130(wt, in, &pos, upwards)) {
            found = true;
            break;
        }
    }

    if (found) {
        if (!wt->takeOut(in)) {
            extern const char linearReduceTakeOutError[];
            reportError(linearReduceTakeOutError);
            return false;
        }
        if (!moveLocks(in, &pos)) {
            reportError("failed to move !\n");
            return false;
        }
        if (!wt->putIn(in)) {
            reportError("failed to put in back in\n");
            return false;
        }
    }
    return true;
}
