// Instruction stream reference lists ("istRefList").
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

const char *linkTypeName(int type);
bool func_0806f4e8(Program *p, char *name);
const char *getUniqueName(Program *p);
char *mprefix_del(const char *a, const char *b);

static inline linkNode *findRef(linkNode *p, instStream *s, int type)
{
    while (p && !(p->unk_0c == s && p->unk_14 == type))
        p = p->unk_08;
    return p;
}

static inline int branchLinkType(unsigned int f)
{
    int t = 0;
    if (f) {
        switch (f & 0xa) {
        case 8: t = 3; break;
        case 2: t = 2; break;
        case 10: t = 4; break;
        default: t = 1; break;
        }
    }
    return t;
}

linkList::~linkList()
{
    linkNode *n = unk_00;
    while (n) {
        if (!unlink(unk_00))
            return;
        delete n;
        n = unk_00;
    }
}

linkNode *linkList::findType(int type)
{
    linkNode *n = unk_00;
    while (n && n->unk_14 != type)
        n = n->unk_08;
    return n;
}

instStream *linkList::getStream(int type)
{
    linkNode *found = 0;
    linkNode *n;
    for (n = unk_00; n; n = n->unk_08) {
        if (n->unk_14 == type) {
            if (found) {
                func_0804ad38("istRefList::getStream  called - but there's more than one ref\n");
                return 0;
            }
            found = n;
        }
    }
    if (found)
        return found->unk_0c;
    func_0804ad38("istRefList: getStream - couldn't find type %d\n", type);
    for (linkNode *p = unk_00; p; p = p->unk_08)
        ;
    return 0;
}

void linkList::dump()
{
    for (linkNode *n = unk_00; n; n = n->unk_08)
        ;
}

bool linkList::removeAll()
{
    linkNode *n = unk_00;
    while (n) {
        if (!unlink(unk_00))
            return false;
        delete n;
        n = unk_00;
    }
    return true;
}

bool linkList::relink(linkNode *n)
{
    if (!n) {
        func_0804ad38("istRefLit:: relink - null ptr\n");
        return false;
    }
    if (n->unk_08 || n->unk_04) {
        func_0804ad38("istRefList :: relink - node is linked\n");
        return false;
    }
    linkNode *p = unk_00;
    instStream *s = n->unk_0c;
    int type = n->unk_14;
    while (p && !(p->unk_0c == s && p->unk_14 == type))
        p = p->unk_08;
    if (p) {
        func_0804ad38("istRefList: already present for relink\n");
        return false;
    }
    if (unk_04) {
        unk_04->unk_08 = n;
        n->unk_04 = unk_04;
    } else
        unk_00 = n;
    unk_04 = n;
    unk_08++;
    return true;
}

bool linkList::unlink(linkNode *n)
{
    if (!n)
        return false;
    if (n->unk_08)
        n->unk_08->unk_04 = n->unk_04;
    else
        unk_04 = n->unk_04;
    if (n->unk_04)
        n->unk_04->unk_08 = n->unk_08;
    else
        unk_00 = n->unk_08;
    n->unk_04 = 0;
    n->unk_08 = 0;
    unk_08--;
    return true;
}

linkNode *linkList::func_0806dff4(instStream *s, int type)
{
    return findRef(unk_00, s, type);
}

bool linkList::addNewRef(instStream *s, int type)
{
    if (findRef(unk_00, s, type)) {
        func_0804ad38("addNewRef - already present\n");
        return false;
    }
    linkNode *n = new linkNode;
    if (!n) {
        func_0804ad38("addNewRef - new failed\n");
        return false;
    }
    n->unk_14 = type;
    n->unk_0c = s;
    if (unk_04) {
        unk_04->unk_08 = n;
        n->unk_04 = unk_04;
    } else
        unk_00 = n;
    unk_04 = n;
    unk_08++;
    return true;
}

bool linkTypeOk(instStream *s, int type)
{
    if (type == 5)
        type = 2;
    return type == branchLinkType(s->unk_2ce) || (type == 0 && (s->unk_2ce & 4));
}

bool Program::linkStreams(instStream *a, instStream *b, int type)
{
    if (!linkTypeOk(a, type)) {
        func_0804ad38("incompatible lnktypes for %s...src=%s\n", linkTypeName(type), a->branchTypeName());
        return false;
    }
    if (!a->unk_134.addNewRef(b, type)) {
        func_0804ad38("LinkStreams - add out failed\n");
        return false;
    }
    if (!b->unk_128.addNewRef(a, type)) {
        func_0804ad38("LinkStreams - add in failed\n");
        return false;
    }
    return true;
}

bool Program::areLinkedBy(instStream *src, instStream *dst, int type)
{
    bool hasSD = findRef(src->unk_134.unk_00, dst, type) != 0;
    bool hasDS = findRef(dst->unk_128.unk_00, src, type) != 0;
    if (hasSD) {
        if (hasDS)
            return true;
    } else if (!hasDS)
        return false;
    func_0804ad38("areLinkedBy - hasSD=%d hasDS=%d src=%s dst=%s\n",
                  hasSD, hasDS, src->func_0806d674(), dst->func_0806d674());
    func_0804adf0(-1);
    return false;
}

bool Program::unlinkStreams(instStream *a, instStream *b, int type)
{
    linkNode *n = findRef(a->unk_134.unk_00, b, type);
    if (!n) {
        func_0804ad38("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      a->func_0806d674(), b->func_0806d674(), type);
        for (linkNode *p = a->unk_134.unk_00; p; p = p->unk_08)
            ;
        return false;
    }
    if (!a->unk_134.unlink(n)) {
        func_0804ad38("unlink streams - failed to unlink out ref\n");
        return false;
    }
    n = findRef(b->unk_128.unk_00, a, type);
    if (!n) {
        func_0804ad38("unLinkStreams::failed to find ref for unlink in input of %s\n", b->func_0806d674());
        for (linkNode *p = b->unk_128.unk_00; p; p = p->unk_08)
            ;
        return false;
    }
    if (!b->unk_128.unlink(n)) {
        func_0804ad38("unlink streams - failed to unlink in ref\n");
        return false;
    }
    return true;
}

bool Program::unLinkStreams(instStream *a, instStream *b, int type)
{
    linkNode *n = findRef(a->unk_134.unk_00, b, type);
    if (!n) {
        func_0804ad38("unLinkStreams::failed to find ref for unlink in output of %s looking for %s type %d\n",
                      a->func_0806d674(), b->func_0806d674(), type);
        for (linkNode *p = a->unk_134.unk_00; p; p = p->unk_08)
            ;
        return false;
    }
    n->unk_11 = 0;
    n = findRef(b->unk_128.unk_00, a, type);
    if (!n) {
        func_0804ad38("unLinkStreams::failed to find ref for unlink in input of %s\n", b->func_0806d674());
        for (linkNode *p = b->unk_128.unk_00; p; p = p->unk_08)
            ;
        return false;
    }
    n->unk_11 = 0;
    return true;
}

int linkList::func_0806e4f0()
{
    int n = 0;
    for (linkNode *p = unk_00; p; p = p->unk_08)
        if (p->unk_11)
            n++;
    return n;
}

int linkNode::func_0806e514()
{
    return unk_0c ? unk_0c->unk_2c0 : -1;
}

int func_0806e530(int flags)
{
    return branchLinkType(flags);
}

const char *linkTypeName(int type)
{
    const char *s = "IFT_UNKNOWN";
    switch (type) {
    case -1: s = "IFT_BAD"; break;
    case 0: s = "IFT_FALL"; break;
    case 1: s = "IFT_ABS"; break;
    case 2: s = "IFT_REG"; break;
    case 3: s = "IFT_ABS_LINK"; break;
    case 4: s = "IFT_REG_LINK"; break;
    case 5: s = "IFT_REG_RET"; break;
    }
    return s;
}

