#ifndef PROGRAM_H
#define PROGRAM_H

#include "mList.h"
#include "Processor.h"
#include "VisitCache.h"
#include "infoList.h"
#include "ubsList.h"

struct CpuDef;
class Parser;
class instStream;

// Flow object ("flowMon") living on main's stack. Name is a guess.
struct Program {
    Program(bool symbolic);
    ~Program();
    bool Init(CpuDef *cpu, const char *uniqueName, bool uniqueAll);
    bool dispose();
    bool addStream(instStream *stream);
    bool insertStreamAfter(instStream *after, instStream *stream);
    bool decodeAndSplitStreams(Parser *parser, int buildType, bool newSyntax);

    Processor proc;
    int emulatedCycleCount;
    char unk_84[4];
    mList uniqueNamePrefixes;       // names prefixed by doUnique
    bool symbolicOutput;
    char unk_9d[3];
    instStream *streamHead; // all streams, chained via unk_108
    instStream *streamTail; // stream list tail
    int streamCount;         // stream count
    short flowIdCount;       // flow UID count
    char unk_ae[2];
    instStream **streamsById; // linear table indexed by stream id
    bool trackMaximumPathLength;
    char traceStoreEnabled;
    char unk_b6[2];
    int traceStoreWordCount;
    VisitCache backTraceVisitCache;  // back-trace visit cache
    instStream *traceTargetStream;
    instStream *matchedStreams;
    instStream *sourceStreams;
    instStream *resolvedStreams;
    instStream *activeStreams;
    char unk_e8[8];

    bool linkStreams(instStream *from, instStream *to, int type);
    bool areLinkedBy(instStream *src, instStream *dst, int type);
    bool unlinkStreams(instStream *from, instStream *to, int type);
    bool unLinkStreams(instStream *from, instStream *to, int type);
    bool doUnique();
    bool hasStreamLabel(const char *name);
    bool removeStream(instStream *stream);
    instStream *findStreamById(int streamId);
    instStream *findStream(const char *name);
    bool retarget(instStream *from, instStream *to);
};

#endif
