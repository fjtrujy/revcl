#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

bool aaFixup(instStream *s);
bool func_08069e7c(Instr *in);

static inline void initImm(Instr *in)
{
    in->unk_13d4 = 0;
    in->unk_13d5 = 0;
    in->unk_13d6 = -1;
    in->unk_13d8.clear();
    in->unk_13d2 = -1;
}

static inline bool Init_immFixup(instStream *s)
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
            if (!Init_immFixup(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!aaFixup(s))
                return dumpFailed(s);
    } else {
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!do_immFixup(s))
                return false;
        for (s = p->unk_a0; s; s = s->unk_108)
            if (!Init_immFixup(s))
                return false;
    }
    return true;
}

static inline bool bit0(unsigned char m)
{
    return m & 1;
}

static inline bool hasAaData(Instr *in)
{
    bool r = true;
    if (in->unk_13d4 == 0 && in->unk_13d5 == 0 && in->unk_13d6 == -1)
        r = in->unk_13d8.isValid();
    return r;
}

bool aaFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("Init_immFixup - failed to get work table\n");
        return false;
    }
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            func_0804ad38("aaFixup - regref had no register\n");
            func_0804adf0(-1);
        }
        Instr *in = (Instr *)ru->unk_1c;
        aaDesc *aa = func_0806c768(in);
        olList *vis = &ru->unk_20[0].unk_00;
        if (!aa || !bit0(ru->unk_0c) || !bit0(ru->unk_0d) || ru->unk_18 != 1)
            continue;
        RegPoolEntry *outReg = in->unk_38[aa->unk_06].unk_18;
        if (outReg != reg || in->unk_38[aa->unk_05].unk_18 != outReg)
            continue;
        ol_s *ol;
        for (ol = vis->func_08058370(); ol; ol = ol->func_080583ac()) {
            Instr *vi = ol->unk_04;
            Operand *vop = func_0805802c(ol);
            if (!vi || !vop) {
                func_0804ad38("failed to get instruction for visible input\n");
                func_0804adf0(-1);
            }
            aaUse *u = func_0806c7f8(vi);
            if (u && ol->unk_10 == u->unk_04 && vop->unk_07 == 1) {
                mValue *v = &vi->unk_13d8;
                if (hasAaData(vi)) {
                    func_0804ad38("instruction already has aa data loaded .. mode before\n");
                    vi->func_08054008(false);
                    v->isValid();
                    return false;
                }
                vi->unk_13d5 = 1;
                vi->unk_13d4 = aa->unk_04;
                vi->unk_13d6 = u->unk_04;
                if (aa->unk_07 < 0)
                    v->setInt(aa->unk_08);
                else if (!v->set(&in->unk_38[aa->unk_07].unk_1c)) {
                    func_0804ad38("failed to set xaaImm for before\n");
                    return false;
                }
            }
        }
        Operand *out = in->getOp(aa->unk_06);
        if (!out || out->unk_09 != 2 || !bit0(out->unk_07)) {
            func_0804ad38("failed to get output operand for aa\n");
            func_0804adf0(-1);
        }
        for (ol = out->unk_2c[0].unk_54.func_08058370(); ol; ol = ol->func_080583ac()) {
            Instr *vi = ol->unk_04;
            Operand *vop = func_0805802c(ol);
            if (!vi || !vop) {
                func_0804ad38("failed to get instruction for visible input.. mode after\n");
                func_0804adf0(-1);
            }
            aaUse *u = func_0806c7f8(vi);
            if (u && ol->unk_10 == u->unk_04 && vop->unk_07 == 1) {
                mValue *v = &vi->unk_13d8;
                if (hasAaData(vi)) {
                    func_0804ad38("instruction already has aa data loaded.. mode after\n");
                    vi->func_08054008(false);
                    v->isValid();
                    return false;
                }
                vi->unk_13d5 = 2;
                vi->unk_13d4 = aa->unk_04;
                vi->unk_13d6 = u->unk_04;
                if (aa->unk_07 < 0)
                    v->setInt(aa->unk_08);
                else if (!v->set(&in->unk_38[aa->unk_07].unk_1c)) {
                    func_0804ad38("failed to set xaaImm after\n");
                    return false;
                }
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
