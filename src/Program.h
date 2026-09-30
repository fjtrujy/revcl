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
    bool addStream(instStream *s);
    bool insertStreamAfter(instStream *after, instStream *s);
    bool func_0806efc4(Parser *parser, bool a, bool newSyntax);

    Processor proc;
    int unk_80;         // emulated length
    char unk_84[4];
    mList unk_88;       // names prefixed by doUnique
    bool unk_9c;        // ctor arg
    char unk_9d[3];
    instStream *unk_a0; // all streams, chained via unk_108
    instStream *unk_a4; // stream list tail
    int unk_a8;         // stream count
    char unk_ac[4];
    char *unk_b0;
    bool unk_b4;        // logmax: also compute max path key
    char unk_b5;
    char unk_b6[2];
    int unk_b8;
    VisitCache unk_bc;  // back-trace visit cache
    instStream *unk_d4; // t target
    instStream *unk_d8; // new match list, chained via unk_254
    instStream *unk_dc; // sources, chained via unk_250
    instStream *unk_e0; // chained via unk_258
    instStream *unk_e4; // chained via unk_25c
    char unk_e8[8];

    bool linkStreams(instStream *from, instStream *to, int type);
    bool areLinkedBy(instStream *src, instStream *dst, int type);
    bool unlinkStreams(instStream *from, instStream *to, int type);
    bool unLinkStreams(instStream *from, instStream *to, int type);
    bool doUnique();
    bool func_0806e878(const char *name);
    bool removeStream(instStream *s);
};

#endif
