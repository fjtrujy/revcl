#include "wtQueue_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);


const char *currentOptimizerName;

struct wrangleCand;
bool selectDumbSchedulingCandidate(workTable *wt, wrangleCand *c);
bool selectLoopDumbSchedulingCandidate(workTable *wt, wrangleCand *c);
bool optNormal1(workTable *wt, wrangleCand *c);
bool optHDown(workTable *wt, wrangleCand *c);
bool optVuta(workTable *wt, wrangleCand *c);
bool optDUp(workTable *wt, wrangleCand *c);
bool optNormal(workTable *wt, wrangleCand *c);
bool optAnother(workTable *wt, wrangleCand *c);
bool optVuta1(workTable *wt, wrangleCand *c);
bool optNormal2(workTable *wt, wrangleCand *c);
bool optDumb2(workTable *wt, wrangleCand *c);

optimiser optTable[11] = {
    {"dumb", 1, 3, (wrangleFunc)selectDumbSchedulingCandidate, 0},
    {"ldumb", 1, 3, (wrangleFunc)selectLoopDumbSchedulingCandidate, 0},
    {"normal1", 1, 1, (wrangleFunc)optNormal1, 0},
    {"hDown", 1, 1, (wrangleFunc)optHDown, 0},
    {"vuta", 1, 3, (wrangleFunc)optVuta, 0},
    {"dUp", 1, 1, (wrangleFunc)optDUp, 0},
    {"normal", 1, 3, (wrangleFunc)optNormal, 0},
    {"another", 1, 3, (wrangleFunc)optAnother, 0},
    {"vuta1", 0, 3, (wrangleFunc)optVuta1, 0},
    {"normal2", 0, 3, (wrangleFunc)optNormal2, 0},
    {"dumb2", 0, 1, (wrangleFunc)optDumb2, 0},
};

int getNumOptimisers()
{
    return 11;
}

static inline void dumpLinearStream(instStream *stream)
{
    stream->dump(0);
}

bool doLinear(Program *program, int smart)
{
    instStream *s;
    while ((s = selectUninsertedStream(program)) != 0) {
        if (s->getCodeMode() != 1) {
            reportError("doLinear() - asked to process a non-uta\n");
            return false;
        }
        if (s->getAllocMode() != 2) {
            reportError("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                          s->streamName(), s->getAllocMode());
            return false;
        }
        if (!prepExtInfo(program, s, 0)) {
            reportError("failed to prep external info for stream\n");
            return false;
        }
        workTable *scheduleTable = s->getTable();
        if (!scheduleTable) {
            reportError("flowMon::Optimise() - couldn't get work table for %s!\n", s->streamName());
            return false;
        }
        if (scheduleTable->getLoopSize()) {
            reportError("loop size set for linear table opt %s\n", s->streamName());
            return false;
        }
        workTable *best = 0;
        int i = smart ? 10 : 0;
        for (; i >= 0 && (i != 0 || !best); i--) {
            optimiser *optimizer = &optTable[i];
            if (optimizer->flags & 2) {
                currentOptimizerName = optimizer->name;
                workTable *nt = prepUta(program, s, 0, 1);
                if (!nt) {
                    reportError("failed to prepUta istream\n");
                    return false;
                }
                nt->optimizerIndex = i;
                if (!statReset(program)) {
                    reportError("failed to reset stats for cpu\n");
                    return false;
                }
                if (!dynCalcStats(nt)) {
                    reportError("dyn calc stats failed for linear\n");
                    return false;
                }
                bool ok = false;
                switch (optimizer->wmode) {
                case 0:
                    ok = scheduleMidTableMode0(nt, optimizer->func);
                    break;
                case 1:
                    ok = scheduleMidTableMode1(nt, optimizer->func);
                    break;
                default:
                    reportError("bad wmode\n");
                    abortAfterError();
                }
                if (!ok)
                    reportWarning(" WARNING: failed to %s via %s\n", optimizer->name, s->streamName());
                else {
                    if (!reduceExtents(nt)) {
                        reportError("failed to reduce extents for code!\n");
                        return false;
                    }
                    if (!fiddleBDL(nt)) {
                        reportError("reduceEx - failed to fiddle bdl\n");
                        return false;
                    }
                    if (!nt->doPostOpt(1)) {
                        reportError("failed to validate/patch moves - so continue\n");
                        exitAfterCleanup(-1);
                    }
                    if (assignLocalRegisters(program, nt)) {
                        if (best) {
                            if (nt->getPhysicalLen() < best->getPhysicalLen()) {
                                delete best;
                                best = nt;
                            } else
                                delete nt;
                        } else
                            best = nt;
                        if (best && best->getPhysicalLen() <= s->getRecordedDependencyLength())
                            break;
                        continue;
                    }
                }
                delete nt;
            }
        }
        if (!best) {
            reportError("no opt table.. something failed making table .. for %s!\n", s->streamName());
            dumpLinearStream(s);
            return false;
        }
        int len = best->getPhysicalLen();
        if (len < 0) {
            reportError("optTable was bad size\n");
            return false;
        }
        if (!s->setOptTable(best)) {
            reportError("set opt table - failed \n");
            return false;
        }
        reportInfo("Opt::Linear %16s  %d/%d\n", s->streamName(), len, s->getRecordedDependencyLength());
    }
    return true;
}

static inline const char *loopStallReason(instStream *stream)
{
    return stallReasonStr(stream->savedStallReason);
}

bool doLoop(Program *program, int smart)
{
    for (instStream *s = program->streamHead; s; s = s->nextStream) {
        int i;
        if (!s->canLoop() || !s->knownMinimumLoopCount)
            continue;
        workTable *best = 0;
        if (!prepExtInfo(program, s, 1)) {
            reportError("failed to prep external info for stream\n");
            return false;
        }
        int size = s->getRecordedLogicalLength();
        size += s->extraLoopCount;
        if (s->absoluteLoopCount > 1) {
            if (size > s->absoluteLoopCount)
                reportWarning("the minimal possible loop size + --LoopExtra was greather than the --LoopAbs setting, so ignoring --LoopAbs\n");
            else
                size = s->absoluteLoopCount;
        }
        bool found = false;
        do {
            s->optimizationRecordCount = 11;
            if (!makeLpWork(s, size)) {
                reportError("failed to generate a work table for loop size %d\n", size);
                return false;
            }
            bool first = true;
            for (i = 10; i >= 0; i--) {
                s->optimizationRecords[i].optimalLength = -1;
                s->optimizationRecords[i].instructionCount = -1;
                s->optimizationRecords[i].candidateLoopId = -1;
                s->optimizationRecords[i].mainLoopId = -1;
                currentOptimizerName = optTable[i].name;
                if (optTable[i].flags & 1) {
                    const char *why = loopStallReason(s);
                    reportInfo("lpgen[%d](limit: %s) - '%s' flow %s\n",
                                  size, why, optTable[i].name, s->streamName());
                    workTable *scheduleTable = prepUta(program, s, size, 1);
                    if (!scheduleTable) {
                        reportError("failed to prepUta istream\n");
                        return false;
                    }
                    scheduleTable->optimizerIndex = i;
                    if (!statReset(program)) {
                        reportError("failed to reset stats for cpu\n");
                        return false;
                    }
                    if (!dynCalcStats(scheduleTable)) {
                        reportError("failed to calc stats for loop\n");
                        return false;
                    }
                    if (first)
                        first = false;
                    bool ok = false;
                    switch (optTable[i].wmode) {
                    case 0:
                        ok = scheduleMidTableMode0(scheduleTable, optTable[i].func);
                        break;
                    case 1:
                        ok = scheduleMidTableMode1(scheduleTable, optTable[i].func);
                        break;
                    default:
                        reportError("bad wmode\n");
                        abortAfterError();
                    }
                    if (ok) {
                        if (!reduceExtents(scheduleTable)) {
                            reportError("failed to reduce extents for code!\n");
                            exitAfterCleanup(-1);
                        }
                        if (!scheduleTable->doPostOpt(2)) {
                            reportError("failed to validate/patch moves - so continue\n");
                            exitAfterCleanup(-1);
                        }
                        if (!assignLocalRegisters(program, scheduleTable))
                            reportWarning("failed to assign reg size would be %d!\n", scheduleTable->getLoopSize());
                        else {
                            s->optimizationRecords[i].optimalLength = s->getRecordedLogicalLength();
                            s->optimizationRecords[i].mainLoopId = scheduleTable->mainLoopId;
                            s->optimizationRecords[i].candidateLoopId = scheduleTable->getCondLid();
                            s->optimizationRecords[i].instructionCount = scheduleTable->getLoopSize();
                            if (best && best->mainLoopId < scheduleTable->mainLoopId) {
                                delete scheduleTable;
                                scheduleTable = 0;
                            }
                            if (scheduleTable) {
                                if (best)
                                    delete best;
                                found = true;
                                best = scheduleTable;
                            }
                            continue;
                        }
                    }
                    delete scheduleTable;
                }
            }
            if (!found) {
                if (!disposeLpWork(s)) {
                    reportError("failed to unget work for loop table\n");
                    return false;
                }
            } else if (!keepLpWork(s)) {
                reportError("failed to fix the new work table for loop opt\n");
                return false;
            }
            size++;
        } while (!found);
        if (!best) {
            reportError("loop code failed for %s\n", s->streamName());
            return false;
        }
        if (!s->setOptTable(best)) {
            reportError("setOptTable failed\n");
            return false;
        }
        reportInfo("Opt::Loop %16s  %d/%d\n", s->streamName(), best->getLoopSize(), s->getRecordedLogicalLength());
    }
    return true;
}

bool doRawAndSynth(Program *program, int smart)
{
    for (instStream *s = program->streamHead; s; s = s->nextStream) {
        if (s->getCodeMode() == 3) {
            if (!prepExtInfo(program, s, 0)) {
                reportError("failed to prep external info for stream\n");
                return false;
            }
            if (!s->getTable()) {
                reportError("work table not set\n");
                exitAfterCleanup(-1);
            }
            if (!optReset(program)) {
                reportError("failed to optReset cpu\n");
                exitAfterCleanup(-1);
            }
            workTable *scheduleTable = makeForceTable(program, s);
            if (!scheduleTable) {
                s->dump(0);
                reportError("failed to prepUta istream\n");
                return false;
            }
            if (!scheduleTable->doPostOpt(s->getRawSchedulingMode())) {
                reportError("failed to post opt after makeForce\n");
                return false;
            }
            if (scheduleTable->getPhysicalLen() < 0) {
                reportError("optTable was bad size\n");
                return false;
            }
            switch ((unsigned char)scheduleTable->allocationMode) {
            case 2:
                if (!assignLocalRegisters(program, scheduleTable)) {
                    reportError("failed to assign reg for vsm table!\n");
                    dumpProgramStreamsAndRegisters(program, 0);
                    reportError("here is %s\n", s->streamName());
                    return false;
                }
                /* fall through */
            case 3:
                if (!s->setOptTable(scheduleTable)) {
                    reportError("set opt table - failed \n");
                    return false;
                }
                break;
            default:
                reportError("bad alloc mode in dorawandsynth\n");
                return false;
            }
        }
        if (s->getCodeMode() == 4) {
            workTable *wt = s->getTable();
            if (s->getAllocMode() != 2) {
                reportError("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                              s->streamName(), s->getAllocMode());
                return false;
            }
            wt->allocationMode = 3;
        }
    }
    return true;
}

bool relaxOpt(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        switch (stream->getCodeMode()) {
        case 3: {
            workTable *opt = stream->optimizedTable;
            if (!opt) {
                reportError("relaxopt - no opt for %s\n", stream->streamName());
                return false;
            }
            workTable *scheduleTable = stream->getTable();
            if (opt != scheduleTable) {
                reportError("relaxOpt - work and opt not same\n");
                return false;
            }
            scheduleTable->dump();
            for (Instr *instruction = stream->getTable()->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
                instruction->optimizedInstructionImageWord = 0;
                instruction->cloneImage = 0;
                instruction->originalInstructionImageWord = 0;
            }
            stream->optimizedTable = 0;
            break;
        }
        case 4:
            break;
        case 1:
            reportError("uta found at relax time %s\n", stream->streamName());
            return false;
        default:
            reportError("unknown code mode found at relax time %s\n", stream->streamName());
            return false;
        }
    }
    return true;
}

bool optimizeProgramScheduling(Program *program, int smart, bool sizeReduce)
{
    for (instStream *s = program->streamHead; s; s = s->nextStream) {
        if (s->getCodeMode() == 3) {
            if (!prepExtInfo(program, s, 0)) {
                reportError("failed to prep external info for stream\n");
                return false;
            }
            if (!optReset(program)) {
                reportError("failed to optReset cpu\n");
                exitAfterCleanup(-1);
            }
            workTable *scheduleTable = makeForceTable(program, s);
            if (!scheduleTable) {
                reportError("failed to prepUta istream\n");
                return false;
            }
            if (!prepareReductionCandidates(program, s, scheduleTable)) {
                reportError("failed to do initial reduction test\n");
                return false;
            }
            if (s->usesFreeRawLayout()) {
                if (!reduceLinearSchedule(program, scheduleTable, s->allowUpwardReduction, s->allowDownwardReduction)) {
                    reportError("failed to linear reduce table\n");
                    return false;
                }
                if (!sredTable(scheduleTable)) {
                    reportError("failed to prep for sred for %s\n", s->streamName());
                    return false;
                }
            }
            if (!scheduleTable->doPostOpt(s->getRawSchedulingMode())) {
                scheduleTable->dump();
                reportError("failed to post opt after makeForce\n");
                return false;
            }
            if (!s->setOptTable(scheduleTable)) {
                reportError("set opt table - failed \n");
                return false;
            }
        }
    }
    return true;
}

const char *getOptimiserName(int optimizerIndex)
{
    int numOptimisers = 11;
    if (optimizerIndex < 0 || optimizerIndex >= numOptimisers) {
        reportError("asked to get an out of bounds optimiser id\n");
        abortAfterError();
    }
    return optTable[optimizerIndex].name;
}

extern const char sredEarliestAfterError[] = "int_Sred.. tline>iline..earlieset insert is after instruction.. mode is upwards %d>%d mfs=%d\n";
extern const char sredMoveInstError[] = "failed to move inst to tline1\n";
extern const char sredLatestBeforeError[] = "latest possible insert is before instruction..%d<%d!\n";
extern const char sredBadlyPlacedError[] = "badly placed instruction detected\n";
