#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"

const char *pipeNames[16] = {
    "VPU_FMAC", "VPU_IALU", "VPU_MOVE", "VPU_LS", "VPU_EFU",
    "VPU_DIV", "VPU_RAND", "VPU_EXT", "VPU_BRANCH", "0",
};

bool bindExternals(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("no worktable!\n");
            func_0804adf0(-1);
        }
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                RegPoolEntry *reg = op->unk_18;
                if (!reg)
                    continue;
                regUse *ru = wt->unk_14.func_0805b5dc(reg);
                if (!ru) {
                    func_0804ad38("failed to find regref in bind externals\n");
                    return false;
                }
                if (ru->isExternal() || wt->unk_08 == 3) {
                    if (reg->unk_14) {
                        func_0804ad38("found a virtual register on an x operand\n");
                        return false;
                    }
                    if (op->unk_10 && op->unk_10 != reg) {
                        func_0804ad38("operand is forced to base, but not this one\n");
                        return false;
                    }
                    op->unk_10 = reg;
                }
            }
        }
    }
    return true;
}

Instr *addXinFor(instStream *s, RegPoolEntry *r);
Instr *addXoutFor(instStream *s, RegPoolEntry *r);
bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag);
RegPoolEntry *getTagRegister(RegPool *pool, char *name);
int getPoolsByClassID(int cls, RegPool **tab, int max);
RegPool *GetRP(unsigned t);

static inline bool addNewInputInl(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        func_0804ad38("add new input failed\n");
        return false;
    }
    return true;
}

static inline bool addNewOutputInl(Instr *in, RegPoolEntry *r, int bits, bool f)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, f)) {
        func_0804ad38("add new output failed\n");
        return false;
    }
    return true;
}

bool bindDependencies(Program *p)
{
    RegPool *pools[8];
    instStream *s;
    int n = getPoolsByClassID(3, pools, 8);
    if (n < 0) {
        func_0804ad38("bindDependencies() - Processor failed to enumerate hardware register classes\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            int bits = in->getRamBits();
            if (!in->unk_139c)
                continue;
            if (!in->unk_13ce) {
                func_0804ad84("tagString found for non memory instruction\n");
                return false;
            }
            if (bits) {
                char *tag = in->unk_139c;
                RegPool *hw = GetRP(10);
                if (!hw) {
                    func_0804ad38("failed to get a hw pool\n");
                    return false;
                }
                if (hw->unk_0e != 3) {
                    func_0804ad38("umm - not a hardware register\n");
                    return false;
                }
                while (*tag) {
                    while (*tag == ':')
                        tag++;
                    char *end = tag;
                    while (*end && *end != ':')
                        end++;
                    if (*end)
                        *end++ = 0;
                    RegPoolEntry *r = getTagRegister(hw, tag);
                    if (!r) {
                        func_0804ad38("failed to create a tag register\n");
                        return false;
                    }
                    switch ((unsigned char)in->unk_13ce) {
                    case 1:
                        if (!addNewInputInl(in, r, bits)) {
                            func_0804ad38("failed to add input operand for load\n");
                            return false;
                        }
                        break;
                    case 2:
                        if (!addNewOutputInl(in, r, bits, true)) {
                            func_0804ad38("failed to add output operand for store\n");
                            return false;
                        }
                        break;
                    default:
                        func_0804ad38("unknown ramdir!\n");
                        return false;
                    }
                    tag = end;
                }
            }
            delete in->unk_139c;
            in->unk_139c = 0;
        }
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        bool xin = s->unk_2cc == 2;
        bool xout = s->unk_2cd == 3;
        if (xin) {
            if (!addXinFor(s, func_0806c224(0x39))) {
                func_0804ad38("failed to add the fpu sticky flags as xin\n");
                return false;
            }
            if (!addXinFor(s, func_0806c224(0x3a))) {
                func_0804ad38("failed to add the div sticky flags as xin\n");
                return false;
            }
        }
        if (xin || xout) {
            for (int i = 0; i != n; i++) {
                RegPool *rp = pools[i];
                RegPoolEntry *ext = rp->unk_14;
                if (!ext) {
                    func_0804ad38("failed to get EXT for pool %s\n", rp->unk_2c);
                    return false;
                }
                if (xin && !addXinFor(s, ext)) {
                    func_0804ad38("failed to add external input to %s\n", s->func_0806d674());
                    return false;
                }
                if (xout && !addXoutFor(s, ext)) {
                    func_0804ad38("failed to add external output to %s\n", s->func_0806d674());
                    return false;
                }
                for (RegPoolEntry *e = rp->unk_1c; e; e = e->next) {
                    if (xin && !addXinFor(s, e)) {
                        func_0804ad38("failed to add external input to %s\n", s->func_0806d674());
                        return false;
                    }
                    // the original calls addXinFor here too
                    if (xout && !addXinFor(s, e)) {
                        func_0804ad38("failed to add external output to %s\n", s->func_0806d674());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

Instr *addXinFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = func_0806bf74(0xbe);
    if (!info) {
        func_0804ad38("addXinFor - cant get xin_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        func_0804ad38("failed to make xin instruction\n");
        return 0;
    }
    if (in->unk_138b != 1) {
        func_0804ad38("xin doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    Operand *op = &in->unk_38[0];
    if (op->unk_04 != 0x37) {
        func_0804ad38("xin wasn't external\n");
        delete in;
        return 0;
    }
    op->unk_18 = r;
    op->unk_07 = (1 << r->unk_28) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Tail(in)) {
        func_0804ad38("failed to an external input to the tail of list\n");
        return 0;
    }
    return in;
}

Instr *addXoutFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = func_0806bf74(0xbf);
    if (!info) {
        func_0804ad38("addxoutFor - can't get xout_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        func_0804ad38("failed to make xout instruction\n");
        return 0;
    }
    if (in->unk_138b != 1) {
        func_0804ad38("xout doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    Operand *op = &in->unk_38[0];
    if (op->unk_04 != 0x37) {
        func_0804ad38("xout wasn't external\n");
        delete in;
        return 0;
    }
    op->unk_18 = r;
    op->unk_07 = (1 << r->unk_28) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Head(in)) {
        func_0804ad38("failed to an external output to the head of list\n");
        return 0;
    }
    return in;
}

bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag)
{
    if (!r) {
        func_0804ad38("add new output - passed a null rg pointer\n");
        return false;
    }
    if (!bits) {
        func_0804ad38("passed null mod bits for new output register\n");
        return false;
    }
    if (bits & (-1 << r->unk_28)) {
        func_0804ad38("add new output - out of bounds mod bits requested\n");
        return false;
    }
    if (!in->unk_34) {
        func_0804ad38("instruction has no m_Inst\n");
        return false;
    }
    unsigned char n = in->unk_138b;
    int i;
    for (i = 0; i < n; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_08 && op->unk_09 == dir && r == op->unk_18) {
            op->unk_07 |= bits;
            return true;
        }
    }
    if (i == 12) {
        func_0804ad38("ran out of operand space for add new output\n");
        return false;
    }
    Operand *op = &in->unk_38[n];
    op->unk_26 = n;
    op->unk_0b = 1;
    op->unk_09 = dir;
    op->unk_08 = 1;
    op->unk_05 = 0;
    op->unk_0d = 0;
    op->unk_0a = flag;
    op->unk_29 = 0;
    op->unk_18 = r;
    op->unk_07 = bits;
    op->unk_04 = kind;
    in->unk_138b++;
    return in->func_08053fac();
}

bool addNewInput(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        func_0804ad38("add new input failed\n");
        return false;
    }
    return true;
}

bool addNewOutput(Instr *in, RegPoolEntry *r, int bits, int flag)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, flag)) {
        func_0804ad38("add new output failed\n");
        return false;
    }
    return true;
}

RegPoolEntry *getTagRegister(RegPool *pool, char *name)
{
    if (RegPool::isResvd(name, pool->unk_0c)) {
        func_0804ad84("can't use %s as a name for one of the %s\n", name, pool->unk_2c);
        return 0;
    }
    RegPoolEntry *r = pool->func_08059a1c(name);
    if (!r) {
        r = pool->func_08059a5c(name);
        if (!r) {
            func_0804ad38("failed to create a virtual reg\n");
            return 0;
        }
    }
    r->unk_14 = false;
    r->unk_28 = 4;
    return r;
}

bool func_08085a9c(Program *)
{
    return true;
}

const char *getUniqueName(Program *p);
int mstrlen(const char *s);
bool expandRawTables(Program *p);
bool resolveStreamBranches(Program *p);
bool recalcFlowUIDs(Program *p);
bool buildUta(Program *prog, instStream *s, flowBlock *blocks, int count);
bool buildSynth(Program *prog, instStream *s, flowBlock *blocks, int count);

bool func_08085aa8(Program *p, flowBlock *blocks, int count, bool newSyntax)
{
    int i = 0;
    if (!blocks) {
        func_0804ad38("flowMon::Build - passed a null table\n");
        return false;
    }
    if (count <= 0) {
        func_0804ad38("flowMon::Build - strange number of entries in the input table\n");
        return false;
    }

    for (; i != count; i++) {
        flowBlock *b = &blocks[i];
        for (int k = 0; k != 2; k++) {
            Instr *in = b->unk_0c[k];
            if (!in || !in->unk_1388)
                continue;
            char *target = in->getBranchTargetImmediate();
            if (target) {
                flowBlock *f = func_0808435c(blocks, count, target);
                if (!f) {
                    func_0804ad84("vcl couldn't find the label '%s' refered to in instruction :\n", target);
                    in->leDump(-1);
                    return false;
                }
                f->unk_04 = 1;
            } else if (in->unk_139c) {
                char *tag = in->unk_139c;
                while (*tag) {
                    char buf[2064];
                    char *d = buf;
                    while (*tag != ':')
                        *d++ = *tag++;
                    *d = 0;
                    flowBlock *f = func_0808435c(blocks, count, buf);
                    if (!f) {
                        func_0804ad38("failed to find a tagged label '%s' attached to branch:\n", buf);
                        in->leDump(-1);
                        return false;
                    }
                    if (!p->unk_88.func_0804b234(buf) && !p->unk_88.func_0804b2d4(buf)) {
                        func_0804ad38("failed to add tag name '%s' to the global tag list\n", buf);
                        return false;
                    }
                    f->unk_04 = 1;
                    tag++;
                }
            }
        }
    }

    int delay = 0;
    int lastUid = 0;
    unsigned char mode = 0;
    unsigned char sub = 0;
    short uid = 1;
    for (i = 0; i != count; i++) {
        flowBlock *b = &blocks[i];
        char kind = b->unk_05;
        switch ((unsigned char)kind) {
        case 1:
        case 3:
            if (mode != (unsigned char)kind || sub != (unsigned char)b->unk_06) {
                uid++;
                mode = kind;
                sub = b->unk_06;
            }
            break;
        case 6:
            uid++;
            break;
        case 0:
            func_0804ad38("flow::Build : hit a bad ientry.at line %d\n", i);
            b->func_08084268(i);
            return false;
        default:
            func_0804ad38("calculateFlowAndGLobalUID's : hit a totally bogus ientry\n");
            return false;
        }
        if (i && b[-1].unk_05 == 6)
            uid++;
        if (b->unk_14.func_0804b794())
            uid++;
        short f = b->unk_0a;
        if (f > 0) {
            if (mode != 3) {
                func_0804ad38("+ve flow uid in non raw sequence\n");
                return false;
            }
            if (f != lastUid) {
                uid++;
                lastUid = f;
            }
        } else if (lastUid > 0) {
            lastUid = 0;
            uid++;
        }
        if (b->unk_04)
            uid++;
        int id = uid;
        for (int k = 0; k != 2; k++) {
            Instr *in = b->unk_0c[k];
            if (!in)
                continue;
            int serial = g_080b33a8++;
            in->unk_13e0 = serial;
            char t = in->unk_13cd;
            switch ((unsigned char)t) {
            case 1:
            case 3:
            case 10:
            case 11:
                uid++;
                id++;
                break;
            case 2:
            case 4:
            case 5:
                uid++;
                break;
            }
            if ((t || mode == 1) && delay) {
                func_0804ad84("found a branch delay slot with no instruction in in RAW mode..\n");
                func_0804ad84("to make life easier for itself - vcl treats this as an error..\n");
                func_0804ad84("this includes.. having a BDL slot as a branch target.. in this case\n");
                return false;
            }
            if (in->unk_1388) {
                if (delay) {
                    func_0804ad84("found a branch in a branch delay slot\n");
                    in->leDump();
                    return false;
                }
                delay = (mode == 3) + 1;
            }
        }
        if (delay) {
            if (delay == 1)
                uid++;
            delay--;
        }
        b->unk_08 = id;
    }
    p->unk_ac = uid;

    mList unused;
    i = 0;
    while (i < count) {
        flowBlock *b = &blocks[i];
        int end = i;
        short id = b->unk_08;
        mode = b->unk_05;
        sub = b->unk_06;
        for (flowBlock *q = b; end < count; end++, q++) {
            if (q->unk_08 != id)
                break;
            if (q->func_080843d0()) {
                if (mode != 1) {
                    func_0804ad84("mismatched synth instruction encountered\n");
                    return false;
                }
                mode = 4;
            }
        }
        int n = end - i;
        instStream *s = new instStream;
        if (!s) {
            func_0804ad38("flowMon::build - new failed\n");
            return false;
        }
        s->unk_2c0 = id;
        mList *labels = &b->unk_14;
        if (!labels->unk_08) {
            char *name = new char[mstrlen(getUniqueName(p)) + 128];
            if (!name) {
                func_0804ad38("failed to allocate new label\n");
                return false;
            }
            func_0804adbc(name, "__v_%s_%d", getUniqueName(p), id);
            if (!s->unk_110.func_0804b444(name)) {
                func_0804ad38("failed to creat add a default name node\n");
                delete[] name;
            }
            delete[] name;
        } else if (!s->unk_110.func_0804b4d0(labels)) {
            func_0804ad38("failed to grab entries from in at top\n");
            return false;
        }
        s->unk_2d4 |= s->unk_110.func_0804b7b4();
        switch (mode) {
        case 1:
            if (!buildUta(p, s, b, n)) {
                func_0804acb0("buildUta table failed for some reason\n");
                return false;
            }
            break;
        case 3:
            if (!buildRaw(p, s, b, n, sub)) {
                func_0804acb0("buildRaw table failed for some reason\n");
                return false;
            }
            break;
        case 4:
            if (!buildSynth(p, s, b, n)) {
                func_0804acb0("buildSynth table failed for some reason\n");
                return false;
            }
            break;
        case 6:
            if (!buildPassThru(p, s, b, n)) {
                func_0804acb0("buildPass failed for some reason\n");
                return false;
            }
            break;
        default:
            func_0804ad38("bad decMode for builder switch\n");
            return false;
        }
        for (int k = 0; k != n; k++)
            blocks[i + k].func_080842f0();
        if (!p->addStream(s)) {
            func_0804ad38("flowBuild: failed to add istream\n");
            return false;
        }
        s->unk_218 = &s->unk_148;
        i = end;
    }
    if (!expandRawTables(p)) {
        func_0804ad38("failed to expand raw tables\n");
        return false;
    }
    if (!resolveStreamBranches(p)) {
        func_0804ad38("failed to resolve branches after constructing streams\n");
        return false;
    }
    if (p->proc.unk_60 && !p->doUnique()) {
        func_0804ad38("failed to do unique processing\n");
        return false;
    }
    if (!bindDependencies(p)) {
        func_0804ad38("bindDependencies1 failed\n");
        return false;
    }
    if (!recalcFlowUIDs(p)) {
        func_0804ad38("failed to recalc flow uids\n");
        return false;
    }
    return true;
}
