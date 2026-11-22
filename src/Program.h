#ifndef PROGRAM_H
#define PROGRAM_H

#include "mList.h"
#include "Processor.h"
#include "VisitCache.h"
#include "infoList.h"
#include "ubsList.h"

struct CpuDef;
class Parser;
class InstructionStream;

// Flow object ("flowMon") living on main's stack. Name is a guess.
struct Program {
    Program(bool symbolic);
    ~Program();
    bool initializeProgram(CpuDef *cpu, const char *uniqueName, bool uniqueAll);
    bool dispose();
    bool addStream(InstructionStream *stream);
    bool insertStreamAfter(InstructionStream *after, InstructionStream *stream);
    bool decodeAndSplitStreams(Parser *parser, int buildType, bool newSyntax);

    Processor proc;
    int emulatedCycleCount;
    char reservedProgramProcessorBytes[4];
    mList uniqueNamePrefixes;       // names prefixed by doUnique
    bool symbolicOutput;
    char reservedProgramOutputBytes[3];
    InstructionStream *streamHead; // all streams, chained via nextStream
    InstructionStream *streamTail; // stream list tail
    int streamCount;         // stream count
    short flowIdCount;       // flow UID count
    char reservedProgramStreamBytes[2];
    InstructionStream **streamsById; // linear table indexed by stream id
    bool trackMaximumPathLength;
    char traceStoreEnabled;
    char reservedProgramTracingBytes[2];
    int traceStoreWordCount;
    VisitCache backTraceVisitCache;  // back-trace visit cache
    InstructionStream *traceTargetStream;
    InstructionStream *matchedStreams;
    InstructionStream *sourceStreams;
    InstructionStream *resolvedStreams;
    InstructionStream *activeStreams;
    char reservedProgramTailBytes[8];

    bool linkStreams(InstructionStream *from, InstructionStream *to, int type);
    bool areLinkedBy(InstructionStream *sourceStream, InstructionStream *destinationStream, int type);
    bool unlinkStreams(InstructionStream *from, InstructionStream *to, int type);
    bool unLinkStreams(InstructionStream *from, InstructionStream *to, int type);
    bool doUnique();
    bool hasStreamLabel(const char *name);
    bool removeStream(InstructionStream *stream);
    InstructionStream *findStreamById(int streamId);
    InstructionStream *findStream(const char *name);
    bool retarget(InstructionStream *from, InstructionStream *to);
};

#endif
