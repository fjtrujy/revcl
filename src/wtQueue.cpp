#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int unk_10;
};

extern optimiser optTable[11];
extern const char *g_080de730;

instStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, instStream *s, bool b);
workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(workTable *wt);
bool func_0807f360(workTable *wt, wrangleFunc f);
bool func_0808061c(workTable *wt, wrangleFunc f);
bool reduceExtents(workTable *wt);
bool fiddleBDL(workTable *wt);
bool assignLocalRegisters(Program *p, workTable *wt);
bool makeLpWork(instStream *s, int maxSpread);
bool disposeLpWork(instStream *s);
bool keepLpWork(instStream *s);
bool optReset(Program *p);
workTable *makeForceTable(Program *p, instStream *s);
void func_0806ed78(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(workTable *wt);
bool func_0807e528(Program *p, instStream *s, workTable *wt);
bool func_0807e740(Program *p, workTable *wt, unsigned char a, unsigned char b);

bool workTable::qRegRef(regUse *ru)
{
    if (!ru) {
        func_0804ad38("qRegRef: passed a null pointer\n");
        func_0804adf0(-1);
    }
    regUse *c = ru->Clone();
    if (!c) {
        func_0804ad38("failed to clone regref\n");
        func_0804adf0(-1);
    }
    RegPoolEntry *reg = c->unk_08;
    if (!reg) {
        func_0804ad38("no register attached to cloned rf during qRegRef\n");
        delete c;
        return false;
    }
    if (!unk_14.append(c)) {
        func_0804ad38("failed to add cloned regref to list\n");
        delete c;
        return false;
    }
    if (~c->unk_0f & c->unk_0d) {
        func_0804ad38("xobits set, but o bits not\n");
        c->func_0805b51c();
        return false;
    }
    if (~c->unk_0e & c->unk_0c) {
        func_0804ad38("xibits set, but i bits not\n");
        c->func_0805b51c();
        return false;
    }
    for (int i = 0; i != reg->unk_28; i++) {
        c->unk_20[i].unk_30 = ru->unk_20[i].unk_30;
        c->unk_20[i].unk_32 = ru->unk_20[i].unk_32;
        cls0805c244 *ll = getLockList(reg, i);
        if (!ll) {
            func_0804ad38("failed to get lock list ptr in register for rfv for reg %s\n", reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        if (c->unk_20[i].unk_58) {
            func_0804ad38("rll is already set in rrp\n");
            func_0804adf0(-1);
        }
        c->unk_20[i].unk_58 = (int)ll;
        if (reg->func_0805a908(i)) {
            func_0804ad38("regref register part already had locklist\n");
            func_0804adf0(-1);
        }
        if (!reg->func_0805a924(i, (int)ll)) {
            func_0804ad38("failed to set curr lock list pointer in rf register\n");
            func_0804adf0(-1);
        }
        if ((c->unk_0f >> i) & 1)
            ll->unk_29 = 0;
        char xi = (c->unk_0c >> i) & 1;
        char t = (c->unk_10 >> i) & 1;
        bool sync = false;
        if (reg->unk_2c && g_080b336c == 0)
            sync = true;
        char nb = reg->unk_1c;
        if (!nb && (c->unk_10 & (c->unk_0f | c->unk_0d))) {
            func_0804ad38("t bits set for nbid register\n");
            c->func_0805b51c();
            return false;
        }
        if (t || xi || sync) {
            unsigned char nbz = nb == 0;
            cls0805bcc4 *lk = c->unk_20[i].func_0805b830();
            if (!lk)
                func_0804ad38("failed to get lock for %s.%d as %s\n",
                              reg->func_0805a8e0(), i, reg->func_0805a8e0());
            if (!lk->setOwner(i, c)) {
                func_0804ad38("falied to set owner\n");
                func_0804adf0(-1);
                return false;
            }
            if (!ll->addOutput(lk, 0, 1, nbz)) {
                func_0804ad38("failed to register output lock for regref\n");
                func_0804adf0(-1);
                return false;
            }
            if (t) {
                bool ext = !(((c->unk_0f | c->unk_0e) >> i) & 1);
                if (!ll->addExtOutput(lk, ext)) {
                    func_0804ad38("failed to add pass thru reg ref as ext output for register %s\n",
                                  reg->func_0805a8e0());
                    return false;
                }
                dump();
            }
            if (!ll->addOutputLock(lk)) {
                func_0804ad38("regRef::LoadLocks - failed to add output lock\n");
                return false;
            }
        }
    }
    return true;
}

bool setPipeBits(Instr *in);
bool AddToDo_Tail(Block *blk, Instr *n);
extern int g_080b33f0;

static inline bool isAAOperand(Operand *op)
{
    return op->unk_2b == 2;
}

static inline unsigned char operandLockMode(Operand *op)
{
    return op->unk_2b;
}

bool workTable::qInstruction(Instr *in)
{
    int i;
    if (!in->func_080541fc()) {
        func_0804ad38("tried to queue a synthetic\n");
        return false;
    }
    in->func_08054138();
    Instr *c = in->clone();
    if (!c) {
        func_0804ad38("failed to clone instruction\n");
        func_0804adf0(-1);
    }
    c->func_08055f78(in);
    if (!unk_10) {
        func_0804ad38("qInstruction - no owner\n");
        return false;
    }
    if (c->unk_13d5 == 1 || c->unk_13d5 == 2) {
        if (c->unk_13d6 < 0) {
            func_0804ad38("m_aaOp<0 but dstMode is set!\n");
            func_0804adf0(-1);
        }
        c->unk_38[c->unk_13d6].unk_2b = 2;
    }
    setPipeBits(c);

    for (i = 0; i != c->unk_138b; i++) {
        Operand *src = in->getOp(i);
        Operand *dst = c->getOp(i);
        if (!src || !dst) {
            func_0804ad38("qInstruction failed to get an operand\n");
            return false;
        }
        RegPoolEntry *reg = dst->unk_18;
        if (!reg)
            continue;
        regUse *rr = unk_14.func_0805b5dc(reg);
        if (!rr) {
            func_0804ad38("find reg ref failed to find rr from owner.. by now - this is fatal.. register %s\n",
                          reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        if (isRegisterOperand(dst, 1)) {
            if (isAAOperand(dst))
                dst->unk_09 = 3;
        }
        if (isRegisterOperand(dst, 1)) {
            if (g_080b336c && func_0806c6c4(reg) && dst->unk_2b != 2)
                dst->unk_2b = 1;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((dst->unk_07 >> k) & 1) {
                cls0805bcc4 *lk = &dst->unk_2c[k];
                int a = 0;
                cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(k);
                if (!ll) {
                    func_0804ad38("failed to get curr lock list during qinst\n");
                    func_0804adf0(-1);
                }
                lk->setOwner(k, c, i);
                cls0805bcc4 *out;
                if ((dst->unk_27 >> k) & 1) {
                    regUse *o = unk_14.func_0805b5dc(reg);
                    if (!o) {
                        func_0804ad38("failed to find regref owner for input\n");
                        return false;
                    }
                    out = o->unk_20[k].func_0805b830();
                } else {
                    olList *uses = &src->unk_2c[k].unk_54;
                    ol_s *ol = uses->unk_00;
                    if (!ol || ol->unk_00) {
                        func_0804ad38("more or less than one oploc for non-extern input\n");
                        func_0804adf0(-1);
                    }
                    Instr *setter = ol->unk_04;
                    if (!setter) {
                        func_0804ad38("could not get setting op for setter\n");
                        func_0804adf0(-1);
                    }
                    Instr *img = setter->unk_08;
                    if (!img) {
                        func_0804ad38("qInstruction - setter had null work clone set\n");
                        return false;
                    }
                    if ((workTable *)img->unk_30 != this) {
                        func_0804ad38("qInstruction  - image of setter wasn't in this table\n");
                        return false;
                    }
                    Operand *sop = img->getOp(ol->unk_10);
                    if (!sop) {
                        func_0804ad38("failed to get setter operand\n");
                        return false;
                    }
                    out = &sop->unk_2c[k];
                    a = dst->unk_29;
                }
                lk->unk_19 = operandLockMode(dst);
                if (!ll->addInput(out, lk, a, dst->unk_0b)) {
                    func_0804ad38("failed to register input operand\n");
                    return false;
                }
                }
            }
        }
        if (isRegisterOperand(dst, 2) && dst->unk_2b != 2 && g_080b336c && func_0806c6c4(reg) && !rr->unk_0d)
            dst->unk_2b = 1;
    }

    for (i = 0; i != c->unk_138b; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = registerOperand(op, 2);
        if (!reg)
            continue;
        for (int k = 0; k != reg->unk_28; k++) {
            if ((op->unk_07 >> k) & 1) {
                cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(k);
                if (!ll) {
                    func_0804ad38("couldn't get rll from reg\n");
                    func_0804adf0(-1);
                }
                cls0805bcc4 *lk = &op->unk_2c[k];
                lk->setOwner(k, c, i);
                int a = 1;
                if (reg->unk_2c)
                    a = c->unk_13cf;
                if (!ll->addOutput(lk, a, op->unk_0a, op->unk_0b)) {
                    func_0804ad38("failed to register output operand : %s.%d\n", reg->func_0805a8e0(), k);
                    return false;
                }
                if ((op->unk_27 >> k) & 1) {
                    cls0805c244 *el = (cls0805c244 *)reg->func_0805a908(k);
                    if (!el)
                        func_0804ad38("couldn't get curr lock list for output\n");
                    if (!el->addExtOutput(lk, false)) {
                        func_0804ad38("failed to add ext output!\n");
                        return false;
                    }
                }
            }
        }
    }

    for (i = 0; i != c->unk_138b; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        Operand *o2;
        for (int j = 0; (o2 = c->getOp(j)) != 0; j++) {
            if (registerOperand(o2, 2) != reg)
                continue;
            int k = 0;
            if (k != reg->unk_28) {
                OpSlot *outputLock = o2->unk_2c;
                OpSlot *inputLock = op->unk_2c;
                do {
                    if (((op->unk_07 & o2->unk_07) >> k) & 1) {
                        if (!inputLock->setInOut(outputLock)) {
                            func_0804ad38("failed to set in out\n");
                            func_0804adf0(-1);
                            return false;
                        }
                    }
                    k++;
                    outputLock++;
                    inputLock++;
                } while (k != reg->unk_28);
            }
        }
    }

    if (!AddToDo_Tail((Block *)this, c)) {
        func_0804ad38("iTable::qInstruction - failed to add to todo tail\n");
        delete c;
        return false;
    }
    if (!logAddLinear_Tail(c)) {
        func_0804ad38("iTable::qInstruction - failed to add to linear list tail\n");
        return false;
    }
    if (unk_38 && c->unk_1388) {
        g_080b33f0 += 2;
        c->unk_1398 = g_080b33f0;
        reserveEntry(g_080b33f0, unk_38 - 2, 1);
    }
    return true;
}

