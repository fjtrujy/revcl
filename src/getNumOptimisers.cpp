#include "wtQueue_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(ScheduleTable *, int *);


const char *currentOptimizerName;

struct wrangleCand;
bool selectDumbSchedulingCandidate(ScheduleTable *wt, wrangleCand *c);
bool selectLoopDumbSchedulingCandidate(ScheduleTable *wt, wrangleCand *c);
bool optNormal1(ScheduleTable *wt, wrangleCand *c);
bool optHDown(ScheduleTable *wt, wrangleCand *c);
bool optVuta(ScheduleTable *wt, wrangleCand *c);
bool optDUp(ScheduleTable *wt, wrangleCand *c);
bool optNormal(ScheduleTable *wt, wrangleCand *c);
bool optAnother(ScheduleTable *wt, wrangleCand *c);
bool optVuta1(ScheduleTable *wt, wrangleCand *c);
bool optNormal2(ScheduleTable *wt, wrangleCand *c);
bool optDumb2(ScheduleTable *wt, wrangleCand *c);

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

int getOptimizerCount()
{
    return 11;
}

static inline void dumpLinearStream(InstructionStream *stream)
{
    stream->dump(0);
}

bool scheduleLinearStreams(Program *program, int smart)
{
    InstructionStream *selectedStream;
    while ((selectedStream = selectUninsertedStream(program)) != 0) {
        if (selectedStream->getCodeMode() != 1) {
            reportError("doLinear() - asked to process a non-uta\n");
            return false;
        }
        if (selectedStream->getAllocMode() != 2) {
            reportError("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                          selectedStream->streamName(), selectedStream->getAllocMode());
            return false;
        }
        if (!prepExtInfo(program, selectedStream, 0)) {
            reportError("failed to prep external info for stream\n");
            return false;
        }
        ScheduleTable *scheduleTable = selectedStream->getTable();
        if (!scheduleTable) {
            reportError("flowMon::Optimise() - couldn't get work table for %s!\n", selectedStream->streamName());
            return false;
        }
        if (scheduleTable->getLoopSize()) {
            reportError("loop size set for linear table opt %s\n", selectedStream->streamName());
            return false;
        }
        ScheduleTable *best = 0;
        int optimizerIndex = smart ? 10 : 0;
        for (; optimizerIndex >= 0 && (optimizerIndex != 0 || !best); optimizerIndex--) {
            optimiser *optimizer = &optTable[optimizerIndex];
            if (optimizer->flags & 2) {
                currentOptimizerName = optimizer->name;
                ScheduleTable *candidateTable = prepUta(program, selectedStream, 0, 1);
                if (!candidateTable) {
                    reportError("failed to prepUta istream\n");
                    return false;
                }
                candidateTable->optimizerIndex = optimizerIndex;
                if (!statReset(program)) {
                    reportError("failed to reset stats for cpu\n");
                    return false;
                }
                if (!dynCalcStats(candidateTable)) {
                    reportError("dyn calc stats failed for linear\n");
                    return false;
                }
                bool schedulingSucceeded = false;
                switch (optimizer->wmode) {
                case 0:
                    schedulingSucceeded = scheduleMidTableMode0(candidateTable, optimizer->func);
                    break;
                case 1:
                    schedulingSucceeded = scheduleMidTableMode1(candidateTable, optimizer->func);
                    break;
                default:
                    reportError("bad wmode\n");
                    abortAfterError();
                }
                if (!schedulingSucceeded)
                    reportWarning(" WARNING: failed to %s via %s\n", optimizer->name, selectedStream->streamName());
                else {
                    if (!reduceExtents(candidateTable)) {
                        reportError("failed to reduce extents for code!\n");
                        return false;
                    }
                    if (!adjustTrailingBranchLayout(candidateTable)) {
                        reportError("reduceEx - failed to fiddle bdl\n");
                        return false;
                    }
                    if (!candidateTable->doPostOpt(1)) {
                        reportError("failed to validate/patch moves - so continue\n");
                        exitAfterCleanup(-1);
                    }
                    if (assignLocalRegisters(program, candidateTable)) {
                        if (best) {
                            if (candidateTable->getPhysicalLen() < best->getPhysicalLen()) {
                                delete best;
                                best = candidateTable;
                            } else
                                delete candidateTable;
                        } else
                            best = candidateTable;
                        if (best && best->getPhysicalLen() <= selectedStream->getRecordedDependencyLength())
                            break;
                        continue;
                    }
                }
                delete candidateTable;
            }
        }
        if (!best) {
            reportError("no opt table.. something failed making table .. for %s!\n", selectedStream->streamName());
            dumpLinearStream(selectedStream);
            return false;
        }
        int len = best->getPhysicalLen();
        if (len < 0) {
            reportError("optTable was bad size\n");
            return false;
        }
        if (!selectedStream->setOptTable(best)) {
            reportError("set opt table - failed \n");
            return false;
        }
        reportInfo("Opt::Linear %16s  %d/%d\n", selectedStream->streamName(), len, selectedStream->getRecordedDependencyLength());
    }
    return true;
}

static inline const char *loopStallReason(InstructionStream *stream)
{
    return stallReasonStr(stream->savedStallReason);
}

bool scheduleLoopStreams(Program *program, int smart)
{
    for (InstructionStream *currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        int optimizerIndex;
        if (!currentStream->canLoop() || !currentStream->knownMinimumLoopCount)
            continue;
        ScheduleTable *best = 0;
        if (!prepExtInfo(program, currentStream, 1)) {
            reportError("failed to prep external info for stream\n");
            return false;
        }
        int size = currentStream->getRecordedLogicalLength();
        size += currentStream->extraLoopCount;
        if (currentStream->absoluteLoopCount > 1) {
            if (size > currentStream->absoluteLoopCount)
                reportWarning("the minimal possible loop size + --LoopExtra was greather than the --LoopAbs setting, so ignoring --LoopAbs\n");
            else
                size = currentStream->absoluteLoopCount;
        }
        bool found = false;
        do {
            currentStream->optimizationRecordCount = 11;
            if (!makeLpWork(currentStream, size)) {
                reportError("failed to generate a work table for loop size %d\n", size);
                return false;
            }
            bool first = true;
            for (optimizerIndex = 10; optimizerIndex >= 0; optimizerIndex--) {
                currentStream->optimizationRecords[optimizerIndex].optimalLength = -1;
                currentStream->optimizationRecords[optimizerIndex].instructionCount = -1;
                currentStream->optimizationRecords[optimizerIndex].candidateLoopId = -1;
                currentStream->optimizationRecords[optimizerIndex].mainLoopId = -1;
                currentOptimizerName = optTable[optimizerIndex].name;
                if (optTable[optimizerIndex].flags & 1) {
                    const char *why = loopStallReason(currentStream);
                    reportInfo("lpgen[%d](limit: %s) - '%s' flow %s\n",
                                  size, why, optTable[optimizerIndex].name, currentStream->streamName());
                    ScheduleTable *scheduleTable = prepUta(program, currentStream, size, 1);
                    if (!scheduleTable) {
                        reportError("failed to prepUta istream\n");
                        return false;
                    }
                    scheduleTable->optimizerIndex = optimizerIndex;
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
                    bool schedulingSucceeded = false;
                    switch (optTable[optimizerIndex].wmode) {
                    case 0:
                        schedulingSucceeded = scheduleMidTableMode0(scheduleTable, optTable[optimizerIndex].func);
                        break;
                    case 1:
                        schedulingSucceeded = scheduleMidTableMode1(scheduleTable, optTable[optimizerIndex].func);
                        break;
                    default:
                        reportError("bad wmode\n");
                        abortAfterError();
                    }
                    if (schedulingSucceeded) {
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
                            currentStream->optimizationRecords[optimizerIndex].optimalLength = currentStream->getRecordedLogicalLength();
                            currentStream->optimizationRecords[optimizerIndex].mainLoopId = scheduleTable->mainLoopId;
                            currentStream->optimizationRecords[optimizerIndex].candidateLoopId = scheduleTable->getTerminatingConditionalLoopId();
                            currentStream->optimizationRecords[optimizerIndex].instructionCount = scheduleTable->getLoopSize();
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
                if (!disposeLpWork(currentStream)) {
                    reportError("failed to unget work for loop table\n");
                    return false;
                }
            } else if (!keepLpWork(currentStream)) {
                reportError("failed to fix the new work table for loop opt\n");
                return false;
            }
            size++;
        } while (!found);
        if (!best) {
            reportError("loop code failed for %s\n", currentStream->streamName());
            return false;
        }
        if (!currentStream->setOptTable(best)) {
            reportError("setOptTable failed\n");
            return false;
        }
        reportInfo("Opt::Loop %16s  %d/%d\n", currentStream->streamName(), best->getLoopSize(), currentStream->getRecordedLogicalLength());
    }
    return true;
}

bool prepareRawAndSyntheticStreams(Program *program, int smart)
{
    for (InstructionStream *currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (currentStream->getCodeMode() == 3) {
            if (!prepExtInfo(program, currentStream, 0)) {
                reportError("failed to prep external info for stream\n");
                return false;
            }
            if (!currentStream->getTable()) {
                reportError("work table not set\n");
                exitAfterCleanup(-1);
            }
            if (!optReset(program)) {
                reportError("failed to optReset cpu\n");
                exitAfterCleanup(-1);
            }
            ScheduleTable *scheduleTable = makeForceTable(program, currentStream);
            if (!scheduleTable) {
                currentStream->dump(0);
                reportError("failed to prepUta istream\n");
                return false;
            }
            if (!scheduleTable->doPostOpt(currentStream->getRawSchedulingMode())) {
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
                    reportError("here is %s\n", currentStream->streamName());
                    return false;
                }
                /* fall through */
            case 3:
                if (!currentStream->setOptTable(scheduleTable)) {
                    reportError("set opt table - failed \n");
                    return false;
                }
                break;
            default:
                reportError("bad alloc mode in dorawandsynth\n");
                return false;
            }
        }
        if (currentStream->getCodeMode() == 4) {
            ScheduleTable *currentOptimizationTable = currentStream->getTable();
            if (currentStream->getAllocMode() != 2) {
                reportError("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                              currentStream->streamName(), currentStream->getAllocMode());
                return false;
            }
            currentOptimizationTable->allocationMode = VCL_ALLOC_DONE;
        }
    }
    return true;
}

bool relaxOpt(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        switch (stream->getCodeMode()) {
        case 3: {
            ScheduleTable *opt = stream->optimizedTable;
            if (!opt) {
                reportError("relaxopt - no opt for %s\n", stream->streamName());
                return false;
            }
            ScheduleTable *scheduleTable = stream->getTable();
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
    for (InstructionStream *currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (currentStream->getCodeMode() == 3) {
            if (!prepExtInfo(program, currentStream, 0)) {
                reportError("failed to prep external info for stream\n");
                return false;
            }
            if (!optReset(program)) {
                reportError("failed to optReset cpu\n");
                exitAfterCleanup(-1);
            }
            ScheduleTable *scheduleTable = makeForceTable(program, currentStream);
            if (!scheduleTable) {
                reportError("failed to prepUta istream\n");
                return false;
            }
            if (!prepareReductionCandidates(program, currentStream, scheduleTable)) {
                reportError("failed to do initial reduction test\n");
                return false;
            }
            if (currentStream->usesFreeRawLayout()) {
                if (!reduceLinearSchedule(program, scheduleTable, currentStream->allowUpwardReduction, currentStream->allowDownwardReduction)) {
                    reportError("failed to linear reduce table\n");
                    return false;
                }
                if (!sredTable(scheduleTable)) {
                    reportError("failed to prep for sred for %s\n", currentStream->streamName());
                    return false;
                }
            }
            if (!scheduleTable->doPostOpt(currentStream->getRawSchedulingMode())) {
                scheduleTable->dump();
                reportError("failed to post opt after makeForce\n");
                return false;
            }
            if (!currentStream->setOptTable(scheduleTable)) {
                reportError("set opt table - failed \n");
                return false;
            }
        }
    }
    return true;
}

const char *getOptimizerName(int optimizerIndex)
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
