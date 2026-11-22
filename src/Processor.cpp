#include "Processor.h"
#include "Instr.h"
#include "genRegPool.h"
#include "hwRegPool.h"
#include "memRegPool.h"
#include "os.h"
#include "Program.h"

extern int errorOccurred;
extern bool quiet_080bbd98;

bool func_0804bf28(char a, char b);
bool func_0804bf9c(char c);
char *dupString(const char *s);

Processor::Processor()
{
    uniqueName = 0;
    g_080b33a8 = 0;
    unk_00 = 0;
    g_080de6c0 = 0;
    unk_1c = "Nameless";
    unk_18 = "";
    unk_64 = 0;
    unk_65 = 0;
    unk_44 = -1;
    instructionsPerRawLine = 0;
    unk_60 = 0;
    unk_3d = 0;
    unk_3e = 0;
    for (int i = 0; i != 12; i++)
        g_080de6e0[i] = 0;
    for (int i = 0; i != 3; i++)
        g_080de714[i] = 0;
}

Processor::~Processor()
{
}

bool Processor::Init(CpuDef *cpu)
{
    if (!cpu) {
        reportError("null pointer got into init for a cpu\n");
        return false;
    }
    if (!cpu->name) {
        reportError("must name a cpu\n");
        return false;
    }
    PoolDef *pools = cpu->pools;
    iInfo *iinfos = cpu->iinfos;
    if (!checkTable(iinfos, 0xd6)) {
        reportError("couldnt init cpu '%s' - the idefs were bad\n", cpu->name);
        return false;
    }
    g_080de720 = cpu->devtab;
    instructionsPerRawLine = cpu->instructionsPerRawLine;
    if (instructionsPerRawLine < 1 || instructionsPerRawLine > 2) {
        reportError("processor '%s' has too many instructions per raw line\n", cpu->name);
        return false;
    }
    unk_1c = cpu->name;
    if (!pools) {
        reportError("processor: init - passed a null strap table\n");
        return false;
    }
    if (!iinfos) {
        reportError("passed a null itable\n");
        return false;
    }
    g_080de6c0 = iinfos;
    for (int i = 0; pools[i].type; i++) {
        PoolDef *poolDefinition = &pools[i];
        RegPool *pool;
        switch (poolDefinition->type) {
        case 1:
            pool = new genRegPool;
            break;
        case 2:
            pool = new hwRegPool;
            break;
        case 3:
            pool = new memRegPool;
            break;
        default:
            reportError("Processor::Init - bad type_id found\n");
            return false;
        }
        if (!pool)
            reportError("Processor::Init  - failed to get a regpool '%s'\n", poolDefinition->name);
        if (!pool->Init((VclPtrWord)poolDefinition->prefix, poolDefinition->name, poolDefinition->cls, poolDefinition->virtualRegisterParts)) {
            reportError("failed to init reg pool '%s'\n", poolDefinition->name);
            delete pool;
            return false;
        }
        if (poolDefinition->zeroReg) {
            RegPoolEntry *e = pool->func_08059418(poolDefinition->zeroReg);
            if (!e) {
                reportError("failed to get zero reg for pool %s\n", poolDefinition->name);
                delete pool;
                return false;
            }
            pool->unk_10 = e;
        }
        if (poolDefinition->extReg) {
            RegPoolEntry *e = pool->func_08059418(poolDefinition->extReg);
            if (!e) {
                reportError("failed to get ext reg for pool %s\n", poolDefinition->name);
                delete pool;
                return false;
            }
            pool->unk_14 = e;
        }
        if (g_080de6e0[poolDefinition->cls]) {
            reportError("already initialised reg type %d\n", poolDefinition->cls);
            delete pool;
            return false;
        }
        g_080de6e0[poolDefinition->cls] = pool;
    }
    if (!cpu->dstSpecs) {
        reportError("procDec has no dstSpecs loaded\n");
        return false;
    }
    g_080de728 = cpu->dstSpecs;
    if (!cpu->srcSpecs) {
        reportError("procDec has no srcSpecs loaded\n");
        return false;
    }
    g_080de724 = cpu->srcSpecs;
    return true;
}

bool Processor::func_0806bc84()
{
    unk_4c.func_0804b1b0();
    unk_20.func_0804b1b0();
    unk_04.func_0804b1b0();
    return true;
}

void Processor::func_0806bcb8()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i])
            g_080de6e0[i]->Dump();
}

int Processor::gatherInflight(ol_s **out)
{
    int n = 0;
    for (int i = 0; i != 12; i++) {
        if (g_080de6e0[i]) {
            int r = g_080de6e0[i]->gatherInflight(out + n);
            if (r < 0) {
                reportError("error during inflight gather\n");
                return -1;
            }
            n += r;
        }
    }
    return n;
}

bool Processor::func_0806bd40()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059c4c()) {
            reportError("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

bool Processor::func_0806bd88()
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059c94()) {
            reportError("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

void Processor::func_0806bdd0()
{
    if (!g_080de6c0) {
        reportError("iInfos not set in processor\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    int n = 0;
    for (iInfo *p = g_080de6c0; p->unk_04; p++) {
        p->func_08070080();
        n++;
    }
    if (n != 0xd6) {
        reportError("error processing inst table\n");
        exitAfterCleanup(-1);
    }
    errorOccurred = saved;
}

void func_0806be68(Program *p)
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i])
            g_080de6e0[i]->Dump();
}

bool parseReset(Program *p)
{
    p->proc.unk_63 = false;
    p->proc.unk_04.func_0804b1b0();
    p->proc.unk_20.func_0804b1b0();
    p->proc.unk_4c.func_0804b1b0();
    p->proc.unk_3d = 0;
    p->proc.unk_3e = 0;
    p->proc.unk_62 = 0;
    p->proc.unk_61 = 0;
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_080595d4()) {
            reportError("Processor::parseReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

bool statReset(Program *p)
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_080594c0()) {
            reportError("Processor::statReset() - failed to stat %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

iInfo *func_0806bf74(int id)
{
    return &g_080de6c0[id];
}

iInfo *func_0806bf8c(const char *tok, bool all)
{
    if (!tok) {
        reportError("Proc::FindInstruction() - null token passed\n");
        return 0;
    }
    if (!g_080de6c0) {
        reportError("Proc::FindInstruction() - no instruction set loaded!\n");
        return 0;
    }
    iInfo *p = g_080de6c0;
    if (!all)
        p += 12;
    for (; p->unk_04; p++) {
        const char *s = tok;
        const char *m = p->unk_04;
        while (*s && *m && func_0804bf28(*s, *m)) {
            s++;
            m++;
        }
        if (*m)
            continue;
        if (!*s)
            return p;
        if (*s == '.' && p->unk_0c) {
            do {
                s++;
                if (!*s)
                    return p;
            } while (*s != '.');
        }
    }
    return 0;
}

bool optReset(Program *p)
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059578()) {
            reportError("Processor::optReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

bool Cycle(Program *p, bool flag)
{
    p->unk_80++;
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059828(flag)) {
            reportError("Processor::Cycle() - failed to cycle %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

bool func_0806c0f8(Program *p)
{
    p->unk_80 = 0;
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_08059464()) {
            reportError("Processor::emuReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

inline RegPool *getRP(unsigned t)
{
    if (t < 12)
        return g_080de6e0[t];
    reportWarning("Processor::GetRP - out of range register type (%d)\n", t);
    return 0;
}

RegPoolEntry *getReg(unsigned type, const char *name)
{
    RegPool *pool = getRP(type);
    if (!pool) {
        reportError("getBaseReg from - not find pool %d\n", type);
        exitAfterCleanup(-1);
    }
    return pool->func_08059418(name);
}

RegPoolEntry *getReg(unsigned type, int index)
{
    RegPool *pool = getRP(type);
    if (!pool) {
        reportError("getBaseReg from - not find pool %d\n", type);
        exitAfterCleanup(-1);
    }
    return pool->func_080593e0(index);
}

RegPoolEntry *func_0806c224(int idx)
{
    for (int i = 0; i != 12; i++) {
        RegPool *rp = g_080de6e0[i];
        if (rp) {
            RegPoolEntry *e = rp->func_080593e0(idx);
            if (e)
                return e;
        }
    }
    return 0;
}

RegPoolEntry *getOperandZeroReg(int kind)
{
    RegPool *pool = getRP(func_08057a34(kind));
    if (!pool) {
        reportError("getRP failed for operand getZeroReg\n");
        exitAfterCleanup(-1);
    }
    return pool->unk_10;
}

RegPoolEntry *getZeroReg(unsigned type)
{
    RegPool *pool = getRP(type);
    if (!pool) {
        reportError("getRP failed getZeroReg\n");
        exitAfterCleanup(-1);
    }
    return pool->unk_10;
}

RegPool *GetRP(unsigned t)
{
    if (t > 11) {
        reportWarning("Processor::GetRP - out of range register type (%d)\n", t);
        return 0;
    }
    return g_080de6e0[t];
}

bool flowReset(Program *p)
{
    p->proc.unk_68.clear();
    p->proc.unk_74.clear();
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->func_0805951c()) {
            reportError("Processor::flowReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

void reportUBS(Program *p)
{
    ubsList *ubs = &p->proc.unk_68;
    if (ubs->unk_08) {
        reportInfo("uh-oh, VCL thinks the following operands were used before set:\n");
        ubs->func_080525c8();
    }
}

void reportStalls(Program *p)
{
    infoList *st = &p->proc.unk_74;
    if (st->unk_08) {
        reportInfo("VCL thinks the following instructions caused stalls:\n");
        st->func_080525c8();
    }
}

int checkDispatchEntry(Instr **ie)
{
    if (!ie) {
        reportError("checkDispatchEntry - passed a null pointer\n");
        exitAfterCleanup(-1);
    }
    for (int p = 0; p != 2; p++) {
        Instr *in = GetI(ie, p);
        if (!in)
            continue;
        if (!(((unsigned char)in->unk_138c >> p) & 1))
            return -1;
        for (int i = 0; i != in->operandCount; i++) {
            RegPoolEntry *r = registerOperand(&in->operands[i], 2);
            if (!r)
                continue;
            for (int q = p + 1; q <= 1; q++) {
                Instr *o = GetI(ie, q);
                if (!o)
                    continue;
                if (!(((unsigned char)o->unk_138c >> q) & 1))
                    return -1;
                for (int j = 0; j != o->operandCount; j++) {
                    RegPoolEntry *r2 = registerOperand(&o->operands[j], 2);
                    if (r2 && r2 == r)
                        return -1;
                }
            }
        }
    }
    return 0;
}

bool wouldPair(Instr **ie, Instr *in, unsigned p)
{
    if (!ie || !in || p > 1) {
        reportError("wouldPair - bad args\n");
        exitAfterCleanup(-1);
    }
    int i;
    for (i = 0; i != 2; i++)
        if (ie[i] == in) {
            ie[i] = (Instr *)in->unk_1398;
            break;
        }
    if (ie[p] != (Instr *)in->unk_1398) {
        if (i != 2)
            ie[i] = in;
        return false;
    }
    ie[p] = in;
    int r = checkDispatchEntry(ie);
    ie[p] = (Instr *)in->unk_1398;
    if (i != 2)
        ie[i] = in;
    return r == 0;
}

const char *func_0806c6a0(int dev)
{
    const char **tab = (const char **)g_080de720;
    if (!tab)
        return "NO_DEVTAB";
    if ((unsigned)dev > 8)
        return "UNKNOWN_DEV";
    return tab[dev];
}

bool func_0806c6c4(RegPoolEntry *r)
{
    return r->registerClass == 8 || r->registerClass == 9;
}

bool func_0806c6dc(Program *p)
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->optReset()) {
            reportError("Processor::optReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

static inline int iinfoId(iInfo *op)
{
    if (!op)
        return -1;
    int id = ((char *)op - (char *)g_080de6c0) / sizeof(iInfo);
    if (id < 0)
        return -1;
    if (id >= 0xd6)
        id = -1;
    return id;
}

int func_0806c72c(iInfo *op)
{
    return iinfoId(op);
}

aaDesc *func_0806c768(Instr *in)
{
    aaDesc *res;
    if (!in) {
        res = 0;
        return res;
    }
    if (!in->opcodeInfo) {
        res = 0;
        return res;
    }
    int id = iinfoId(in->opcodeInfo);
    if (id < 0) {
        reportError("couldn't deduce instruction uid\n");
        exitAfterCleanup(-1);
    }
    if (!g_080de724) {
        res = 0;
        return res;
    }
    aaDesc *t = (aaDesc *)g_080de724;
    if (t->unk_00) {
        do {
            if (t->unk_00 == id) {
                res = t;
                return res;
            }
            t++;
        } while (t->unk_00);
    }
    res = 0;
    return res;
}

aaUse *func_0806c7f8(Instr *in)
{
    if (!in)
        return 0;
    if (!in->opcodeInfo)
        return 0;
    int id = iinfoId(in->opcodeInfo);
    if (id < 0) {
        reportError("couldn't deduce instruction uid\n");
        exitAfterCleanup(-1);
    }
    if (!g_080de724)
        return 0;
    for (aaUse *t = (aaUse *)g_080de728; t->unk_00; t++)
        if (t->unk_00 == id)
            return t;
    return 0;
}

bool emitReset(Program *p)
{
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && !g_080de6e0[i]->emitReset()) {
            reportError("Processor::emitReset() - failed to reset %s\n",
                          g_080de6e0[i]->unk_2c);
            return false;
        }
    return true;
}

char *emitStats(Program *p, char *buf)
{
    buf += formatIntoBuffer(buf, "; register stats:\n");
    for (int i = 0; i != 12; i++)
        if (g_080de6e0[i] && g_080de6e0[i]->unk_18) {
            buf = g_080de6e0[i]->emitStats(buf);
            if (!buf) {
                reportError("Processor::emitStats() - failed to emit stats for %s\n",
                              g_080de6e0[i]->unk_2c);
                return 0;
            }
        }
    return buf;
}

int getPoolsByClassID(int cls, RegPool **tab, int max)
{
    if (!tab) {
        reportError("Processor :: passed a null table pointer to getPoolsByClassID()\n");
        return -1;
    }
    int n = 0;
    for (int i = 0; i != 12; i++) {
        RegPool *rp = g_080de6e0[i];
        if (rp && rp->unk_0e == cls) {
            if (n >= max) {
                reportError("Processor:: more pools than space provided in getPoolsByClassID()\n");
                return -1;
            }
            tab[n++] = rp;
        }
    }
    return n;
}

bool Processor::setUniqueName(const char *name)
{
    if (uniqueName)
        delete[] uniqueName;
    uniqueName = 0;
    if (name) {
        uniqueName = dupString(name);
        for (char *s = uniqueName; *s; s++)
            if (!func_0804bf9c(*s))
                *s = '_';
    }
    return true;
}

const char *Processor::getUniqueName()
{
    return uniqueName ? uniqueName : "FLOW";
}

int g_080b33a8 = 1;
bool stagger_080b33ac = false;
iInfo *g_080de6c0;
RegPool *g_080de6e0[13];
VclPtrWord g_080de714[3];
void *g_080de720;
void *g_080de724;
void *g_080de728;
