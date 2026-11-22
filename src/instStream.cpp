#include "os.h"
#include "instStream.h"

bool isReferencedLabel(Program *p, char *name);
bool func_0806f4e8(Program *p, char *name);

instStream::instStream()
{
    unk_290.clear();
    unk_2d8 = 0;
    unk_144 = 0;
    unk_140 = 0;
    unk_2b8 = 0;
    unk_2ba = 0;
    unk_2e0 = -1;
    unk_2e4 = -1;
    unk_2bc = 0x4000;
    unk_2dc = 0;
    unk_268 = 0;
    unk_26c = 0;
    unk_2c4 = 0;
    unk_270 = 0;
    unk_274 = 0;
    unk_2c8 = -1;
    unk_238 = 0;
    unk_2d0 = 0;
    unk_108 = 0;
    unk_10c = 0;
    unk_2c0 = 0;
    unk_2c2 = 0;
    unk_2c6 = 0;
    unk_104 = 0;
    unk_218 = 0;
    unk_220 = 0;
    unk_214 = 0;
    unk_21c = 0;
    unk_124 = 0;
    unk_2d6 = 0;
    unk_2d7 = 0;
    unk_2d4 = 0;
    unk_2d5 = 0;
    unk_260 = 0;
    unk_261 = 0;
    unk_262 = 0;
    unk_2b6 = -1;
    unk_2b4 = -1;
    unk_2ce = 0;
    unk_2cd = 1;
    unk_2cc = 1;
    unk_00 = 0;
}

instStream::~instStream()
{
    reset();
}

bool instStream::reset()
{
    unk_2b8 = 0;
    unk_2ba = 0;
    unk_110.func_0804b1b0();
    unk_2c0 = -1;
    unk_104 = 0;
    unk_2ce = 0;
    if (unk_238)
        delete[] unk_238;
    unk_238 = 0;
    if (unk_2d0)
        delete[] unk_2d0;
    unk_2d0 = 0;
    unk_128.removeAll();
    unk_134.removeAll();
    if (unk_124)
        delete[] unk_124;
    unk_124 = 0;
    return true;
}

void instStream::dump(int)
{
    unk_110.func_0804b534();
    unk_128.dump();
    canLoop();
    workTable *wt = unk_218;
    if (!wt) {
        reportWarning("iStream:: dump - null work table - defaulting to inputTable\n");
        wt = &unk_148;
    }
    wt->dump();
    unk_134.dump();
    int dep = func_0805f284();
    int log = func_0805f29c();
    // length print compiled out in this build
    if (dep > 0 || log > 0) {
    }
}

void instStream::zeroStats()
{
    if (!unk_218) {
        reportError("no work table set for zeroStats!\n");
        exitAfterCleanup(-1);
    }
    for (Instr *i = unk_218->unk_88.unk_00; i; i = i->unk_1c.unk_00)
        i->unk_13f0.func_0805e340();
    unk_278.func_0805e340();
}

bool instStream::iOwn(Instr *i)
{
    if (!i) {
        reportWarning("iStream::iOwn - passed a null pointer\n");
        return false;
    }
    instStream *o = i->func_08054168();
    if (!o) {
        reportWarning("iStream::iOwn - no owner for instruction\n");
        return false;
    }
    return o == this;
}

bool instStream::patchLatencies()
{
    workTable *wt = unk_218;
    int n = 0;
    if (!wt) {
        reportError("iStream::PatchLatencies - no work table\n");
        return false;
    }
    for (Instr *i = wt->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
        i->unk_13e4 = n;
        if (!i->func_08052f84()) {
            reportError("failed to patch latencies for instruction\n");
            return false;
        }
        n++;
    }
    return true;
}

const char *istEndTypeName(int t)
{
    const char *s = "ISTET_BAD";
    switch (t) {
    case 1:
        s = "ISTET_NULL";
        break;
    case 2:
        s = "ISTET_END_CONT";
        break;
    case 3:
        s = "ISTET_END_END";
        break;
    case 4:
        s = "ISTET_END_MACRO";
        break;
    }
    return s;
}

const char *istBranchTypeName(int t)
{
    return func_08055254(t);
}

const char *instStream::branchTypeName()
{
    return func_08055254(unk_2ce);
}

const char *istStartTypeName(int t)
{
    const char *s;
    switch (t) {
    case 1:
        s = "ISTST_NULL";
        break;
    case 2:
        s = "ISTST_ENTRY";
        break;
    default:
        s = "ISTST???\n";
        break;
    }
    return s;
}

const char *instStream::endTypeName()
{
    int t = unk_2cd;
    const char *s = "ISTET_BAD";
    switch (t) {
    case 1:
        s = "ISTET_NULL";
        break;
    case 2:
        s = "ISTET_END_CONT";
        break;
    case 3:
        s = "ISTET_END_END";
        break;
    case 4:
        s = "ISTET_END_MACRO";
        break;
    }
    return s;
}

const char *instStream::startTypeName()
{
    const char *s;
    switch (unk_2cc) {
    case 1:
        s = "ISTST_NULL";
        break;
    case 2:
        s = "ISTST_ENTRY";
        break;
    default:
        s = "ISTST???\n";
        break;
    }
    return s;
}

bool instStream::canLoop()
{
    if (unk_2ce == 1 || unk_2ce == 5) {
        instStream *s = unk_134.getStream(1);
        if (!s) {
            reportError("canLoop - bad return from outIST getFirst\n");
            exitAfterCleanup(-1);
        } else {
            int sv = s->unk_00;
            unsigned char z = s->unk_2ce;
            z >>= 4;
            z >>= 4;
            if (z)
                return false;
            s->unk_00 = sv;
        }
        return s == this;
    }
    return false;
}

static inline void dumpEmitStream(instStream *s)
{
    s->dump(false);
}

char *instStream::emit(char *buf, int a, int *p1, int *p2)
{
    *p1 = 0;
    *p2 = 0;
    if (!buf)
        return 0;
    if (!unk_110.unk_08) {
        reportError("iStream::Emit: stream with no name\n");
        return 0;
    }
    if (!(char)checkStartEnd()) {
        reportError("failed to check stream /start/end\n");
        dumpEmitStream(this);
        return 0;
    }
    if (unk_2cc != 2 && !unk_128.func_0806e4f0() && !unk_124) {
        reportInfo("skip target emit. .coz it's no longer visible\n");
        return buf;
    }
    for (mNode *n = unk_110.unk_0c; n; n = n->unk_04) {
        if (n->unk_14 && (isReferencedLabel(unk_104, n->unk_14)
                          || func_0806f4e8(unk_104, n->unk_14) || n->unk_18))
            buf += formatIntoBuffer(buf, "%s:\n", n->unk_14);
    }
    if (unk_124) {
        buf += formatIntoBuffer(buf, "\n%s\n", unk_124);
    } else {
        if (!unk_21c) {
            reportError("can't emit .. no table!\n");
            return 0;
        }
        buf = unk_21c->emit(buf, a, p1, p2);
        if (!buf) {
            reportError("failed to build out table\n");
            return 0;
        }
        if (unk_2cd == 4) {
            buf += formatIntoBuffer(buf, "\n");
            if (!unk_2d0) {
                reportError("end string not set\n");
                return 0;
            }
            buf += formatIntoBuffer(buf, "         %s\n", unk_2d0);
        }
    }
    return buf;
}

bool instStream::flowReset()
{
    if (!unk_218) {
        reportError("work table not set\n");
        return false;
    }
    if (!unk_218->unk_14.clear()) {
        reportError("iStream::flowReset() - failed to dispose regrefs\n");
        return false;
    }
    for (Instr *i = unk_218->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
        if (!i->func_0805417c()) {
            reportError("failed to init flow for isn\n");
            return false;
        }
    }
    if (!unk_218->func_080737e4()) {
        reportError("failed to flow reset the iEntries for a work table\n");
        return false;
    }
    unk_260 = 0;
    return true;
}

const char *instStream::streamName()
{
    const char *candidateName = unk_110.func_0804b550();
    const char *resolvedName = "UNNAMED_ISTREAM";
    if (candidateName)
        resolvedName = candidateName;
    return resolvedName;
}

void instStream::dumpSynthetics()
{
    checkedTable("dump synthetics .. work table not set\n")->dumpSynthetics();
}

char *instStream::emitLabels(char *buf)
{
    if (unk_2c8 >= 0) {
        for (mNode *n = unk_110.unk_0c; n; n = n->unk_04) {
            if (n->unk_14) {
                const char *p = "__v_";
                const char *s = n->unk_14;
                while (*p && *s && *p == *s) {
                    p++;
                    s++;
                }
                if (*p)
                    buf += formatIntoBuffer(buf, "%-40s = %d\n", n->unk_14, unk_2c8);
            }
        }
    }
    return buf;
}

Instr *instStream::getBranch()
{
    return checkedTable("getBranch  - no work table!\n")->getBranch();
}

bool instStream::removeBranch()
{
    requireTable("getBranch  - no work table!\n");
    if (!unk_218->removeBranch()) {
        reportError("failed to delete branch from work table\n");
        return false;
    }
    unk_2ce = 0;
    return true;
}

char *instStream::getBranchTargetImmediate()
{
    char *r = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *br = unk_218->getBranch();
    if (br)
        r = br->getBranchTargetImmediate();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return r;
}

RegPoolEntry *instStream::getBranchTargetRegister()
{
    RegPoolEntry *r = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *br = unk_218->getBranch();
    if (br)
        r = br->getBranchTargetRegister();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return r;
}

unsigned char instStream::func_0806d908()
{
    workTable *wt = unk_218;
    if (wt)
        return wt->unk_09;
    reportError("getCodeMode - work table not set\n");
    return 0;
}

const char *instStream::getCodeModeName()
{
    workTable *wt = unk_218;
    if (wt)
        return codeModeName(wt->unk_09);
    reportError("getCodeMode - work table not set\n");
    return 0;
}

unsigned char instStream::func_0806d968()
{
    workTable *wt = unk_218;
    if (wt)
        return wt->func_08073754();
    reportError("getCodeMode - work table not set\n");
    return 0;
}

static inline bool isRawTable(workTable *wt)
{
    return wt->unk_09 == 3;
}

static inline bool isFreeRawTable(workTable *wt)
{
    return isRawTable(wt) && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

bool instStream::func_0806d9a0()
{
    workTable *wt = unk_218;
    if (!wt)
        return false;
    return isFreeRawTable(wt);
}

static inline bool isFixedRawTable(workTable *wt)
{
    return isRawTable(wt) && (wt->unk_0a == 2 || wt->unk_0a == 4);
}

bool instStream::func_0806d9d4()
{
    workTable *wt = unk_218;
    if (!wt)
        return false;
    return isFixedRawTable(wt);
}

unsigned char instStream::getAllocMode()
{
    workTable *wt = unk_218;
    if (wt)
        return wt->unk_08;
    reportError("getAllocMode - work table not set\n");
    return 0;
}

bool instStream::func_0806da34()
{
    workTable *wt = unk_218;
    if (wt && wt->unk_09 == 3)
        return false;
    return true;
}

bool instStream::func_0806da58()
{
    if (unk_218)
        return unk_218->func_08073854();
    return false;
}

int instStream::func_0806da80()
{
    int st = unk_2cc;
    int ok = 0;
    if (st == 1 || st == 2) {
        int et = unk_2cd;
        unsigned endTypeCount = 4;
        ok = (unsigned)(et - 1) < endTypeCount;
    }
    return ok;
}

bool instStream::processCmdSynth(Instr *i)
{
    if (!i) {
        reportError("processCmdSynth - passed a null ptr\n");
        return false;
    }
    if (i->unk_1420 > 0) {
        reportUserMessage("parseLid set for processCmdSynth\n");
        i->leDump();
        return false;
    }
    switch ((unsigned char)i->unk_13cd) {
    case 5:
        if (unk_2cd != 1) {
            reportError("CONT directive encountered - but end mode isn't null\n");
            return false;
        }
        unk_2cd = 2;
        break;
    case 8:
        if (!i->func_08054d64(0, &unk_270) || !i->func_08054d64(1, &unk_274)) {
            reportUserMessage("failed to decode the --LoopCS directive, one of the 2 expected operands failed to parse\n");
            i->leDump();
            return false;
        }
        if (unk_270 <= 0) {
            reportUserMessage("for --LoopCS, the minimum known count (arg 1) must be must be >0\n");
            i->leDump();
            return false;
        }
        if (unk_274 < 0) {
            reportUserMessage("for --LoopCS, the minimum safe count (arg 2) must be must be >=0\n");
            i->leDump();
            return false;
        }
        break;
    case 9:
        if (!i->func_08054d64(0, &unk_26c) || unk_26c < 0) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopExtra. value must be >=0\n");
            i->leDump();
            return false;
        }
        break;
    case 10:
        if (unk_2d6) {
            const char *s = unk_110.func_0804b550();
            const char *n = "UNNAMED_ISTREAM";
            if (s)
                n = s;
            reportError("multiple barriers in one flow..%s?\n", n);
            return false;
        }
        unk_2d6 = 1;
        break;
    default:
        reportUserMessage("unexpected synthetic instruction encountered\n");
        break;
    case 12:
        if (!i->func_08054d64(0, &unk_268) || unk_268 <= 2) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopAbs. value must be >=2\n");
            i->leDump();
            return false;
        }
        break;
    }
    return true;
}

bool instStream::disposeInputTable()
{
    workTable *t = &unk_148;
    if (t == unk_218) {
        reportError("can't dispose of input table while it's the work table\n");
        abortAfterError();
    }
    return t->dispose();
}

void instStream::func_0806dc9c(instStream *src)
{
    unk_00 = src->unk_00;
    for (int i = 0; i != unk_00; i++)
        unk_04[i] = src->unk_04[i];
}
