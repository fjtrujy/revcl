// Instruction stream reference lists ("istRefList").
#include "linkList.h"
#include "os.h"

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
