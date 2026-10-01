#ifndef GENREGPOOL_H
#define GENREGPOOL_H

#include "RegPool.h"

struct regUse;

class genRegPool : public RegPool {
public:
    genRegPool();
    virtual ~genRegPool();
    virtual bool assignExt(instStream *s);                        // slot 2
    virtual bool func_08088e6c(instStream *s, RegPoolEntry *reg); // slot 3
    virtual bool func_08088e54(workTable *t);                     // slot 4

    void func_08089a4c();
    bool build_tTab(RegPoolEntry *reg, unsigned char mask);
    bool initAlloc(int start, int end);
    void dumpTTab();
    int countTTab();
    int fitCost(int reg);
    bool set_t_in_aTab(int reg);
    void dumpATab();
    int bestReg();
    bool set_in_aTab(regUse *ru);

    char unk_30[0x20];
};

#endif
