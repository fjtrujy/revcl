#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"
#include "Program.h"
#include "instStream.h"

int reportError(const char *fmt, ...);
bool func_08061c34(instStream *n);
bool extendBackTrace(Program *p, instStream *n);
bool processActiveList(Program *p);
int func_08061cb0(Program *p, instStream *n);

bool tPairList::clear()
{
    while (head) {
        tPair *p = head;
        head = p->next;
        if (p)
            ::operator delete(p);
    }
    count = 0;
    return true;
}

void tPairList::dump()
{
    int n = 0;
    for (tPair *p = head; p; p = p->next)
        n++;
    if (n != count) {
        reportError("     tpairdump : entry count mismatch\n");
        exitAfterCleanup(-1);
    }
}

bool tPairList::addPair(void *a, void *b, int src, int dst)
{
    if (!a || !b) {
        reportError("tPairList::addPair - null arg\n");
        return false;
    }
    if (src <= 0) {
        reportError("tPairList::addKey() src key is bad\n");
        return false;
    }
    if (dst < 0) {
        reportError("tPairList::addKey() dst_key is bad\n");
        return false;
    }
    tPair *p = new tPair(tPair::INIT);
    if (!p) {
        reportError("failed to get a new pair\n");
        return false;
    }
    p->a = a;
    p->b = b;
    p->src = src;
    p->dst = dst;
    p->next = head;
    count++;
    head = p;
    return true;
}

tPair::tPair()
{
    next = 0;
    src = -1;
    dst = -1;
    a = 0;
    b = 0;
}

tPair::~tPair()
{
}

void tPair::reset()
{
    next = 0;
    src = -1;
    dst = -1;
    a = 0;
    b = 0;
}

void tPair::func_08058ab0()
{
}

bool int_buildTpairs(Program *p, RegPoolEntry *reg, int part, instStream *t, void *b)
{
    if (!t) {
        reportError("int_buildTpairs - null arg\n");
        return false;
    }
    p->unk_d4 = t;
    p->unk_dc = 0;
    p->unk_e0 = 0;
    p->unk_e4 = 0;
    p->unk_b5 = 0;
    p->unk_b4 = 0;
    for (instStream *i = p->streamHead; i; i = i->nextStream) {
        if (!func_08061c34(i)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
    }
    if (!extendBackTrace(p, t)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *i = p->unk_e4; i; i = i->unk_25c) {
        i->unk_244 = 0;
        i->unk_24c = 0;
        i->unk_240 = 0;
        i->unk_248 = 0;
    }
    if (!processActiveList(p)) {
        reportError("failed to do internal trace for  a t pair\n");
        return false;
    }
    regUse *tr = t->unk_234;
    if (!tr) {
        reportError("there's  no regref set in t target\n");
        return false;
    }
    if (tr->unk_08 != reg) {
        reportError("there's a mismatch between reg and regref\n");
        return false;
    }
    for (instStream *s = p->unk_dc; s; s = s->unk_250) {
        regUse *sr = s->unk_234;
        if (!sr) {
            reportError("there's no regref in t src\n");
            return false;
        }
        if (sr->unk_08 != reg) {
            reportError("mismatch between regref reg and rg\n");
            return false;
        }
        int dst_key = t->unk_23c;
        int src_key = func_08061cb0(p, s);
        if (dst_key < 0) {
            reportError("dst_key is bad in tpair\n");
            return false;
        }
        if (src_key < 0) {
            reportError("src_key is bad in tpair\n");
            return false;
        }
        if (!tr->unk_20[part].unk_10.addPair(s, b, src_key, dst_key)) {
            reportError("failed to add tpair for tcheck\n");
            return false;
        }
    }
    return true;
}
