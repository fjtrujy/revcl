#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, InstructionPosition *pos);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
void dumpProgramStreamsAndRegisters(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int checkInstructionPlacementConstraints(workTable *wt, Instr *i, InstructionPosition *pos);
bool setRange(int start, int end, int lsz);
int pullUp(workTable *wt);
int dragDown(workTable *wt);

static inline bool isPipeMode(workTable *scheduleTable)
{
    int mode = scheduleTable->codeModeValue - 3;
    return !mode && (scheduleTable->rawSchedulingMode == 1 || scheduleTable->rawSchedulingMode == 3);
}

static inline void dumpLinearReduce(Program *program)
{
    dumpProgramStreamsAndRegisters(program, 0);
}

bool reduceLinearSchedule(Program *program, workTable *scheduleTable, unsigned char up, unsigned char down)
{
    if (!isPipeMode(scheduleTable))
        return true;
    if (scheduleTable->getLoopSize())
        return true;

    int passes = up != 0;
    if (down)
        passes++;

    scheduleTable->getRangeStart();
    scheduleTable->getRangeEnd();
    setRangeFrom(scheduleTable);

    int count = passes;
    while (count) {
        count--;
        if (up) {
            int reductionResult = pullUp(scheduleTable);
            if (reductionResult < 0) {
                reportError("failed to pull up\n");
                dumpLinearReduce(program);
                return false;
            }
            if (reductionResult)
                count = passes;
        }
        if (down) {
            int r = dragDown(scheduleTable);
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

bool doLocalMove(workTable *scheduleTable, Instr *instruction, int s_test, int e_test, int moveDirection)
{
    InstructionPosition pos;
    bool found = false;
    bool upwards;

    if (moveDirection == -1) {
        if (s_test < e_test) {
            reportError("do_localMove - s_test<e_test\n");
            return false;
        }
        upwards = true;
    } else if (moveDirection == 1) {
        if (s_test > e_test) {
            reportError("do_localMove + s_test>e_test\n");
            return false;
        }
        upwards = false;
    } else {
        reportError("bad dl for doLocalMove\t .. %d\n", moveDirection);
        return false;
    }

    for (; s_test != e_test; s_test += moveDirection) {
        pos.setFromLinearRow(s_test);
        if (!checkPlacementInAllowedPipelines(scheduleTable, instruction, &pos, upwards)) {
            found = true;
            break;
        }
    }

    if (found) {
        if (!scheduleTable->takeOut(instruction)) {
            extern const char linearReduceTakeOutError[];
            reportError(linearReduceTakeOutError);
            return false;
        }
        if (!moveLocks(instruction, &pos)) {
            reportError("failed to move !\n");
            return false;
        }
        if (!scheduleTable->putIn(instruction)) {
            reportError("failed to put in back in\n");
            return false;
        }
    }
    return true;
}
