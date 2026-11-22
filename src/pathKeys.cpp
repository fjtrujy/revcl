#include "os.h"
#include "backTrace_shared.h"

bool func_08061c34(instStream *n);

static inline regUseSlot *registerPart(regUse *u, int part)
{
    return &u->unk_20[part];
}

bool func_08061ab0(instStream *n, RegPoolEntry *reg, int part)
{
    workTable *wt = n->getTable();
    if (!wt) {
        reportError("failed to get work table\n");
        return 0;
    }
    regUse *u = wt->unk_14.func_0805b5dc(reg);
    n->unk_227 = 0;
    n->unk_228 = 0;
    n->unk_258 = 0;
    n->unk_234 = u;
    if (u && ((u->unk_0f >> part) & 1)) {
        if (wt->codeMode() == 1 || wt->codeMode() == 4) {
            n->unk_224 = 1;
            n->unk_230 = 0;
            n->unk_232 = 0;
        } else {
            int count = u->unk_20[part].unk_08.unk_04;
            n->unk_224 = count;
            ol_s *head = u->unk_20[part].unk_08.unk_00;
            if (!head) {
                reportError("visible outputs has no head for l_init.. but it should!\n");
                return 0;
            }
            int lo = head->unk_12;
            n->unk_230 = lo;
            ol_s *last = registerPart(u, part)->func_0805ba2c(0, true);
            if (!last) {
                reportError("visible outputs had nothing visible at infinity\n");
                return 0;
            }
            int hi = last->unk_12;
            n->unk_232 = hi;
            if (n->unk_230 > n->unk_232) {
                reportError("minimal writeVal was > maximal writeVal\n");
                return 0;
            }
        }
    } else {
        n->unk_224 = 0;
        n->unk_230 = -1;
        n->unk_232 = -2;
    }
    return func_08061c34(n);
}

bool func_08061c34(instStream *n)
{
    n->unk_226 = 0;
    for (linkNode *l = n->unk_128.unk_00; l; l = l->unk_08)
        l->unk_10 = l->unk_14 != 3 && l->unk_14 != 4;
    n->unk_240 = -1;
    n->unk_244 = 0;
    n->unk_248 = -1;
    n->unk_24c = 0;
    n->unk_250 = 0;
    n->unk_254 = 0;
    n->unk_25c = 0;
    return true;
}

int func_08061cb0(Program *p, instStream *n)
{
    VisitCache *vc = &p->unk_bc;
    if (!vc->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->unk_d4->unk_2c0)) {
        reportError("failed to add target to min path key\n");
        return 0;
    }
    for (; n; n = n->unk_244) {
        if (!vc->setBit(n->unk_2c0)) {
            reportError("failed to add to to min path record\n");
            return 0;
        }
    }
    return vc->endRecord();
}

int getMaxPathKey(Program *p, instStream *n)
{
    VisitCache *vc = &p->unk_bc;
    if (!vc->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->unk_d4->unk_2c0)) {
        reportError("failed to add target to max path key\n");
        return 0;
    }
    for (; n; n = n->unk_24c) {
        if (!vc->setBit(n->unk_2c0)) {
            reportError("failed to add to record to max path\n");
            return 0;
        }
    }
    return vc->endRecord();
}

bool isPathRaw(Program *p, int key)
{
    if (key <= 0) {
        reportError("<=0 input to isPathRaw\n");
        exitAfterCleanup(-1);
    }
    for (instStream *s = p->streamHead; s; s = s->unk_108) {
        int id = s->unk_2c0;
        if (p->unk_bc.testBit(key, id) && s->func_0806d908() == 1)
            return false;
    }
    return true;
}

int guessKeyLen(Program *p, int key, int start, int end)
{
    instStream *s = p->streamHead;
    int len = 0;
    bool foundStart = false;
    bool foundEnd = false;
    for (; s; s = s->unk_108) {
        int id = s->unk_2c0;
        if (p->unk_bc.testBit(key, id)) {
            bool hit = false;
            if (id == start) {
                foundStart = true;
                hit = true;
            }
            if (id == end) {
                foundEnd = true;
                hit = true;
            }
            if (!hit)
                len += s->getOrGuessDepLength();
        }
    }
    if (!foundEnd || !foundStart) {
        reportError("guessKeyLen - didn't contain one of start or end\n");
        return -1;
    }
    return len;
}

void dumpMinPathToTarget(instStream *s)
{
    reportInfo("%s  minPathToTarget\n", s->streamName());
    int i = 0;
    for (instStream *q = s->unk_244; q; q = q->unk_244)
        reportInfo("    %2d:   [%3d] %s\n", ++i, q->unk_240, q->streamName());
    reportInfo("done .. %s  minPathToTarget\n", s->streamName());
}

void dumpMaxPathToTarget(instStream *s)
{
    reportInfo("%s  maxPathToTarget\n", s->streamName());
    int i = 0;
    for (instStream *q = s->unk_24c; q; q = q->unk_24c)
        reportInfo("    %2d:   [%3d] %s\n", ++i, q->unk_248, q->streamName());
    reportInfo("done .. %s  maxPathToTarget\n", s->streamName());
}
