#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

bool aaFixup(instStream *s);
bool func_08069e7c(Instr *in);
int maddImmediate(Operand *op, mValue *v, int n);

static inline void initImm(Instr *in)
{
    in->unk_13d4 = 0;
    in->unk_13d5 = 0;
    in->unk_13d6 = -1;
    in->unk_13d8.clear();
    in->unk_13d2 = -1;
}

static inline bool initStreamImm(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("Init_immFixup - failed to get work table\n");
        return false;
    }
    s->unk_2c2 = 0;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        initImm(in);
    return true;
}

static inline void fixInst(Instr *in)
{
    if (!func_08069e7c(in)) {
        func_0804ad38("do_immFixup - failed for an inst\n");
        func_0804adf0(-1);
    }
}

static inline bool do_immFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt)
        return true;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        fixInst(in);
    return true;
}

static inline bool dumpFailed(instStream *s)
{
    s->dump(false);
    return false;
}

bool func_08069324(Program *p, bool init)
{
    instStream *s;
    if (init) {
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!initStreamImm(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!aaFixup(s))
                return dumpFailed(s);
    } else {
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!do_immFixup(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!initStreamImm(s))
                return false;
    }
    return true;
}


static inline bool bit0(unsigned char m)
{
    return m & 1;
}

static inline bool bit0Clear(unsigned char m)
{
    m ^= 1;
    return m & 1;
}

static inline bool hasAaData(Instr *in)
{
    if (in->unk_13d4 != 0 || in->unk_13d5 != 0 ||
        in->unk_13d6 != -1 || in->unk_13d8.isValid())
        return true;
    return false;
}

static inline bool immValid(mValue *v)
{
    return v->isValid();
}

static inline regUse *firstRegUse(workTable *wt) { return wt->unk_14.unk_04; }
static inline RegPoolEntry *regUseReg(regUse *ru) { return ru->unk_08; }
static inline Instr *regUseInstr(regUse *ru) { return ru->unk_1c; }
static inline olList *regUseVisList0(regUse *ru) { return &ru->unk_20[0].unk_00; }
static inline unsigned char regUseExtIn(regUse *ru) { return ru->unk_0c; }
static inline unsigned char regUseExtOut(regUse *ru) { return ru->unk_0d; }
static inline Operand *instrRawOp(Instr *in, unsigned char idx) { return &in->unk_38[idx]; }
static inline Operand *instrRawOpS(Instr *in, signed char idx) { return &in->unk_38[idx]; }
static inline RegPoolEntry *opRawReg(Operand *op) { return op->unk_18; }
static inline unsigned char opSlotMask(Operand *op) { return op->unk_07; }
static inline mValue *opImmVal(Operand *op) { return &op->unk_1c; }
static inline olList *opSlotUses0(Operand *op) { return &op->unk_2c[0].unk_54; }
static inline Instr *olInstr(ol_s *ol) { return ol->unk_04; }
static inline char olOpIndex(ol_s *ol) { return ol->unk_10; }
static inline mValue *instrAaImm(Instr *in) { return &in->unk_13d8; }
static inline unsigned char aaOutOp(aaDesc *aa) { return aa->unk_06; }
static inline unsigned char aaInOp(aaDesc *aa) { return aa->unk_05; }
static inline signed char aaImmOp(aaDesc *aa) { return aa->unk_07; }
static inline int aaImmLit(aaDesc *aa) { return aa->unk_08; }
static inline unsigned char aaUseOp(aaUse *u) { return u->unk_04; }

bool aaFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("Init_immFixup - failed to get work table\n");
        return false;
    }
    for (regUse *ru = firstRegUse(wt); ru; ru = ru->unk_04) {
        RegPoolEntry *reg = regUseReg(ru);
        if (!reg) {
            func_0804ad38("aaFixup - regref had no register\n");
            func_0804adf0(-1);
        }
        Instr *in = regUseInstr(ru);
        olList *vis = regUseVisList0(ru);
        aaDesc *aa = func_0806c768(in);
        if (!aa || !bit0(regUseExtIn(ru)) || !bit0(regUseExtOut(ru)) || ru->unk_18 != 1)
            continue;
        RegPoolEntry *outReg = in->unk_38[aa->unk_06].unk_18;
        if (outReg != reg || opRawReg(instrRawOp(in, aaInOp(aa))) != outReg)
            continue;
        ol_s *ol;
        for (ol = vis->func_08058370(); ol; ol = ol->func_080583ac()) {
            Instr *vi = olInstr(ol);
            Operand *vop = func_0805802c(ol);
            if (!vi)
                goto bad1;
            if (!vop)
                goto bad2;
        cont:
            {
            aaUse *u = func_0806c7f8(vi);
            if (u && olOpIndex(ol) == aaUseOp(u) && opSlotMask(vop) == 1) {
                if (hasAaData(vi)) {
                    func_0804ad38("instruction already has aa data loaded .. mode before\n");
                    vi->func_08054008(false);
                    immValid(instrAaImm(vi));
                    return false;
                }
                vi->unk_13d5 = 1;
                vi->unk_13d4 = aa->unk_04;
                vi->unk_13d6 = u->unk_04;
                mValue *v = instrAaImm(vi);
                if (aa->unk_07 >= 0) {
                    if (!v->set(opImmVal(instrRawOpS(in, aaImmOp(aa))))) {
                        func_0804ad38("failed to set xaaImm for before\n");
                        return false;
                    }
                } else
                    v->setInt(aaImmLit(aa));
            }
            continue;
            }
        bad1:
            func_0804ad38("failed to get instruction for visible input\n");
            func_0804adf0(-1);
            goto cont;
        bad2:
            func_0804ad38("failed to get instruction for visible input\n");
            func_0804adf0(-1);
            goto cont;
        }
        Operand *out = in->getOp(aaOutOp(aa));
        if (!out || !out->isOutput() || bit0Clear(opSlotMask(out))) {
            func_0804ad38("failed to get output operand for aa\n");
            func_0804adf0(-1);
        }
        for (ol = opSlotUses0(out)->func_08058370(); ol; ol = ol->func_080583ac()) {
            Instr *vi = olInstr(ol);
            Operand *vop = func_0805802c(ol);
            if (!vi || !vop) {
                func_0804ad38("failed to get instruction for visible input.. mode after\n");
                func_0804adf0(-1);
            }
            aaUse *u = func_0806c7f8(vi);
            if (u && olOpIndex(ol) == aaUseOp(u) && opSlotMask(vop) == 1) {
                if (hasAaData(vi)) {
                    func_0804ad38("instruction already has aa data loaded.. mode after\n");
                    vi->func_08054008(false);
                    immValid(instrAaImm(vi));
                    return false;
                }
                mValue *v = instrAaImm(vi);
                vi->unk_13d5 = 2;
                vi->unk_13d4 = aa->unk_04;
                vi->unk_13d6 = u->unk_04;
                if (aa->unk_07 >= 0) {
                    if (!v->set(opImmVal(instrRawOpS(in, aaImmOp(aa))))) {
                        func_0804ad38("failed to set xaaImm after\n");
                        return false;
                    }
                } else
                    v->setInt(aaImmLit(aa));
            }
        }
    }
    return true;
}

bool tidyStream(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt)
        return true;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        fixInst(in);
    return true;
}

bool tidyTable(workTable *wt)
{
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        fixInst(in);
    return true;
}

bool tidyProgram(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108)
        if (!tidyStream(s)) {
            func_0804ad38("failed to tidy a stream\n");
            func_0804adf0(-1);
        }
    return true;
}

struct epSrc {
    short lid;
    short type;
};

bool getSrcEpLid(olList *oll, int luid, int epType, int *lid)
{
    if (luid < 0) {
        func_0804ad38("entered getSrcEpLid with -ve luid\n");
        return false;
    }
    if (!oll->unk_04) {
        func_0804ad38("oll has no entries for getSrcEpLid\n");
        return false;
    }
    int nLocal = 0;
    ol_s *ol;
    for (ol = oll->unk_00; ol; ol = ol->unk_00) {
        Instr *in = ol->unk_04;
        if (!in) {
            func_0804ad38("failed to get in from ol\n");
            func_0804adf0(-1);
        }
        instStream *s = in->func_08054168();
        if (!s) {
            func_0804ad38("getSrcEpLid- failed to get owner\n");
            return false;
        }
        if (s->unk_2c2 != luid) {
            *lid = -1;
            return true;
        }
    }
    epSrc src[2];
    for (ol = oll->unk_00; ol; ol = ol->unk_00) {
        Instr *in = ol->unk_04;
        if (!in) {
            func_0804ad38("failed to get in from ol\n");
            func_0804adf0(-1);
        }
        instStream *s = in->func_08054168();
        if (!s) {
            func_0804ad38("getSrcEpLid- failed to get owner\n");
            return false;
        }
        int type = s->unk_2c4;
        if (s->unk_2c2 != luid) {
            func_0804ad38("getSrcEpLid() - rather unlikely\n");
            return false;
        }
        if (nLocal > 1) {
            func_0804ad38("nLocal got >1 in getSrcEpLid()\n");
            oll->func_08057e9c();
            return false;
        }
        src[nLocal].lid = in->unk_13d2;
        src[nLocal].type = type;
        nLocal++;
    }
    if (nLocal == 0) {
        func_0804ad38("no valid eplids found for getSrc\n");
        return false;
    }
    if (nLocal == 2) {
        if (src[0].type == src[1].type) {
            func_0804ad38("nLocal is 2 , but both setters from same type of construct\n");
            return false;
        }
        if (epType == 2) {
            func_0804ad38("nLocal is 2, but we are a prologue!\n");
            return false;
        }
        for (int i = 0; i != 2; i++) {
            switch (epType) {
            case 3:
                switch (src[i].type) {
                case 2:
                    break;
                case 5:
                    src[i].lid--;
                    break;
                default:
                    func_0804ad38("epSrc: we are EPI, but not MAIN/PRO setter for n=2\n");
                    return false;
                }
                break;
            case 5:
                switch (src[i].type) {
                case 2:
                    break;
                case 5:
                    src[i].lid--;
                    break;
                default:
                    func_0804ad38("epSrc: we are MAIN, but not MAIN/PRO setter\n");
                    return false;
                }
                break;
            default:
                func_0804ad38("bad loop type found for n=2\n");
                return false;
            }
        }
        if (src[0].lid != src[1].lid) {
            func_0804ad38("lid mismatch for n=2\n");
            return false;
        }
    } else {
    switch (epType) {
    case 2:
        break;
    case 3:
        switch (src[0].type) {
        case 2:
        case 3:
            break;
        case 5:
            src[0].lid--;
            break;
        default:
            func_0804ad38("bad ep type for n=1\n");
            return false;
        }
        break;
    case 5:
        if (src[0].type != 2 && src[0].type != 5) {
            func_0804ad38("bad ep type for n=1  %d\n", src[0].type);
            return false;
        }
        break;
    default:
        func_0804ad38("mylm is bad for n=1\n");
        return false;
    }
    }
    *lid = src[0].lid;
    return true;
}

static inline bool isCinput(Operand *op)
{
    return op->unk_09 == 1;
}

bool func_08069e7c(Instr *in)
{
    instStream *s = in->func_08054168();
    if (!s) {
        func_0804ad38("failed to get owner istream from inst during immediate fixup\n");
        func_0804adf0(-1);
    }
    if (!in->unk_13d4)
        return true;
    Operand *op = in->getOp(in->unk_13d6);
    if (!op) {
        func_0804ad38("failed to get opernad for imm fix\n");
        func_0804adf0(-1);
    }
    if (!isCinput(op)) {
        func_0804ad38("operand isn't cinput for in fix\n");
        func_0804adf0(-1);
    }
    int ep;
    if (s->unk_2c2) {
        if (in->unk_13d2 < 0) {
            func_0804ad38("loop, epID<0 for operand in im fixup aaDstMode=%d..epMode=%d\n",
                          in->unk_13d5, in->unk_13d2);
            in->func_08054008(false);
            func_0804adf0(-1);
        }
        ep = in->unk_13d2;
    } else {
        if (in->unk_13d2 >= 0) {
            func_0804ad38("nonloop, epID>=0 for operand in im fixup aaDstmode=%d..epMode=%d\n",
                          in->unk_13d5, in->unk_13d2);
            in->func_08054008(false);
            func_0804adf0(-1);
        }
        ep = 0;
    }
    if (in->unk_13d5 == 1)
        ep--;
    int lid;
    if (s->unk_2c2) {
        olList *oll = &op->unk_2c[0].unk_54;
        if (!getSrcEpLid(oll, s->unk_2c2, s->unk_2c4, &lid)) {
            func_0804ad38("failed to determine actual loopid of setter.. we are:\n");
            in->leDump();
            if (in->func_08054168())
                func_0804ad38("parent stream is : %s\n", in->func_08054168()->func_0806d674());
            func_0804ad38("expected was %d\n", in->unk_13d2);
            func_0804adf0(-1);
        }
    } else
        lid = op->unk_27 ? -1 : 0;
    if (ep != lid) {
    int diff = ep - lid;
    int dir;
    if (diff < 0) {
        dir = -1;
        diff = -diff;
    } else
        dir = 1;
    int mul;
    switch (in->unk_13d4) {
    case 1:
        mul = 1;
        break;
    case 2:
        mul = -1;
        break;
    case 3:
        func_0804ad38("well - I'll code this one later\n");
        func_0804adf0(-1);
        return false;
    default:
        func_0804ad38("bad opchar mode %d\n", in->unk_13d4);
        func_0804adf0(-1);
        return false;
    }
    if (dir * mul < 0)
        diff = -diff;
    int r = maddImmediate(&in->unk_38[in->unk_13d6], &in->unk_13d8, diff);
    if (r < 0) {
        func_0804ad38("failed to madd immediates\n");
        in->leDump(in->unk_13d6);
        func_0804adf0(-1);
    }
    if (r == 1) {
        func_0804ad38("ran out of bits for immfixup\n");
        return false;
    }
    }
    return true;
}

bool Init_immFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("Init_immFixup - failed to get work table\n");
        return false;
    }
    s->unk_2c2 = 0;
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        initImm(in);
    return true;
}

void Instr::hasImmValue()
{
    immValid(&unk_13d8);
}

bool Instr::hasImmFixup()
{
    if (unk_13d4 != 0 || unk_13d5 != 0 || unk_13d6 != -1 || unk_13d8.isValid())
        return true;
    return false;
}

void Instr::resetImmFixup()
{
    unk_13d4 = 0;
    unk_13d5 = 0;
    unk_13d6 = -1;
    unk_13d8.clear();
}

extern const char parseImmediateError[] = "failed to parse immediate '%s'\n";
extern const char evalImmediateError[] = "failed to evaluate tree for '%s' to type %s\n";
extern const char parseImmediateWarn[] = "encountered an error parsing '%s'\n";
extern const char maddImmediateInputError[] = "madd immediate- input not valid\n";
