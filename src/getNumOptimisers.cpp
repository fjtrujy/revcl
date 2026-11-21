#include "wtQueue_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);


int getNumOptimisers()
{
    return 11;
}

static inline void dumpLinearStream(instStream *s)
{
    s->dump(0);
}

bool doLinear(Program *p, int smart)
{
    instStream *s;
    while ((s = selectUninsertedStream(p)) != 0) {
        if (s->func_0806d908() != 1) {
            func_0804ad38("doLinear() - asked to process a non-uta\n");
            return false;
        }
        if (s->getAllocMode() != 2) {
            func_0804ad38("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                          s->func_0806d674(), s->getAllocMode());
            return false;
        }
        if (!prepExtInfo(p, s, 0)) {
            func_0804ad38("failed to prep external info for stream\n");
            return false;
        }
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("flowMon::Optimise() - couldn't get work table for %s!\n", s->func_0806d674());
            return false;
        }
        if (wt->func_080741b4()) {
            func_0804ad38("loop size set for linear table opt %s\n", s->func_0806d674());
            return false;
        }
        workTable *best = 0;
        int i = smart ? 10 : 0;
        for (; i >= 0 && (i != 0 || !best); i--) {
            optimiser *o = &optTable[i];
            if (o->flags & 2) {
                g_080de730 = o->name;
                workTable *nt = prepUta(p, s, 0, 1);
                if (!nt) {
                    func_0804ad38("failed to prepUta istream\n");
                    return false;
                }
                nt->unk_00 = i;
                if (!statReset(p)) {
                    func_0804ad38("failed to reset stats for cpu\n");
                    return false;
                }
                if (!dynCalcStats(nt)) {
                    func_0804ad38("dyn calc stats failed for linear\n");
                    return false;
                }
                bool ok = false;
                switch (o->wmode) {
                case 0:
                    ok = func_0808061c(nt, o->func);
                    break;
                case 1:
                    ok = func_0807f360(nt, o->func);
                    break;
                default:
                    func_0804ad38("bad wmode\n");
                    func_0804aeb0();
                }
                if (!ok)
                    func_0804ac7c(" WARNING: failed to %s via %s\n", o->name, s->func_0806d674());
                else {
                    if (!reduceExtents(nt)) {
                        func_0804ad38("failed to reduce extents for code!\n");
                        return false;
                    }
                    if (!fiddleBDL(nt)) {
                        func_0804ad38("reduceEx - failed to fiddle bdl\n");
                        return false;
                    }
                    if (!nt->doPostOpt(1)) {
                        func_0804ad38("failed to validate/patch moves - so continue\n");
                        func_0804adf0(-1);
                    }
                    if (assignLocalRegisters(p, nt)) {
                        if (best) {
                            if (nt->getPhysicalLen() < best->getPhysicalLen()) {
                                delete best;
                                best = nt;
                            } else
                                delete nt;
                        } else
                            best = nt;
                        if (best && best->getPhysicalLen() <= s->func_0805f284())
                            break;
                        continue;
                    }
                }
                delete nt;
            }
        }
        if (!best) {
            func_0804ad38("no opt table.. something failed making table .. for %s!\n", s->func_0806d674());
            dumpLinearStream(s);
            return false;
        }
        int len = best->getPhysicalLen();
        if (len < 0) {
            func_0804ad38("optTable was bad size\n");
            return false;
        }
        if (!s->setOptTable(best)) {
            func_0804ad38("set opt table - failed \n");
            return false;
        }
        func_0804acb0("Opt::Linear %16s  %d/%d\n", s->func_0806d674(), len, s->func_0805f284());
    }
    return true;
}

static inline const char *loopStallReason(instStream *s)
{
    return stallReasonStr(s->unk_2b8);
}

bool doLoop(Program *p, int smart)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        int i;
        if (!s->canLoop() || !s->unk_270)
            continue;
        workTable *best = 0;
        if (!prepExtInfo(p, s, 1)) {
            func_0804ad38("failed to prep external info for stream\n");
            return false;
        }
        int size = s->func_0805f29c();
        size += s->unk_26c;
        if (s->unk_268 > 1) {
            if (size > s->unk_268)
                func_0804ac7c("the minimal possible loop size + --LoopExtra was greather than the --LoopAbs setting, so ignoring --LoopAbs\n");
            else
                size = s->unk_268;
        }
        bool found = false;
        do {
            s->unk_00 = 11;
            if (!makeLpWork(s, size)) {
                func_0804ad38("failed to generate a work table for loop size %d\n", size);
                return false;
            }
            bool first = true;
            for (i = 10; i >= 0; i--) {
                s->unk_04[i].unk_04 = -1;
                s->unk_04[i].unk_00 = -1;
                s->unk_04[i].unk_0c = -1;
                s->unk_04[i].unk_08 = -1;
                g_080de730 = optTable[i].name;
                if (optTable[i].flags & 1) {
                    const char *why = loopStallReason(s);
                    func_0804acb0("lpgen[%d](limit: %s) - '%s' flow %s\n",
                                  size, why, optTable[i].name, s->func_0806d674());
                    workTable *nt = prepUta(p, s, size, 1);
                    if (!nt) {
                        func_0804ad38("failed to prepUta istream\n");
                        return false;
                    }
                    nt->unk_00 = i;
                    if (!statReset(p)) {
                        func_0804ad38("failed to reset stats for cpu\n");
                        return false;
                    }
                    if (!dynCalcStats(nt)) {
                        func_0804ad38("failed to calc stats for loop\n");
                        return false;
                    }
                    if (first)
                        first = false;
                    bool ok = false;
                    switch (optTable[i].wmode) {
                    case 0:
                        ok = func_0808061c(nt, optTable[i].func);
                        break;
                    case 1:
                        ok = func_0807f360(nt, optTable[i].func);
                        break;
                    default:
                        func_0804ad38("bad wmode\n");
                        func_0804aeb0();
                    }
                    if (ok) {
                        if (!reduceExtents(nt)) {
                            func_0804ad38("failed to reduce extents for code!\n");
                            func_0804adf0(-1);
                        }
                        if (!nt->doPostOpt(2)) {
                            func_0804ad38("failed to validate/patch moves - so continue\n");
                            func_0804adf0(-1);
                        }
                        if (!assignLocalRegisters(p, nt))
                            func_0804ac7c("failed to assign reg size would be %d!\n", nt->func_080741b4());
                        else {
                            s->unk_04[i].unk_04 = s->func_0805f29c();
                            s->unk_04[i].unk_08 = nt->unk_3c;
                            s->unk_04[i].unk_0c = nt->getCondLid();
                            s->unk_04[i].unk_00 = nt->func_080741b4();
                            if (best && best->unk_3c < nt->unk_3c) {
                                delete nt;
                                nt = 0;
                            }
                            if (nt) {
                                if (best)
                                    delete best;
                                found = true;
                                best = nt;
                            }
                            continue;
                        }
                    }
                    delete nt;
                }
            }
            if (!found) {
                if (!disposeLpWork(s)) {
                    func_0804ad38("failed to unget work for loop table\n");
                    return false;
                }
            } else if (!keepLpWork(s)) {
                func_0804ad38("failed to fix the new work table for loop opt\n");
                return false;
            }
            size++;
        } while (!found);
        if (!best) {
            func_0804ad38("loop code failed for %s\n", s->func_0806d674());
            return false;
        }
        if (!s->setOptTable(best)) {
            func_0804ad38("setOptTable failed\n");
            return false;
        }
        func_0804acb0("Opt::Loop %16s  %d/%d\n", s->func_0806d674(), best->func_080741b4(), s->func_0805f29c());
    }
    return true;
}

bool doRawAndSynth(Program *p, int smart)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->func_0806d908() == 3) {
            if (!prepExtInfo(p, s, 0)) {
                func_0804ad38("failed to prep external info for stream\n");
                return false;
            }
            if (!s->getTable()) {
                func_0804ad38("work table not set\n");
                func_0804adf0(-1);
            }
            if (!optReset(p)) {
                func_0804ad38("failed to optReset cpu\n");
                func_0804adf0(-1);
            }
            workTable *wt = makeForceTable(p, s);
            if (!wt) {
                s->dump(0);
                func_0804ad38("failed to prepUta istream\n");
                return false;
            }
            if (!wt->doPostOpt(s->func_0806d968())) {
                func_0804ad38("failed to post opt after makeForce\n");
                return false;
            }
            if (wt->getPhysicalLen() < 0) {
                func_0804ad38("optTable was bad size\n");
                return false;
            }
            switch ((unsigned char)wt->unk_08) {
            case 2:
                if (!assignLocalRegisters(p, wt)) {
                    func_0804ad38("failed to assign reg for vsm table!\n");
                    func_0806ed78(p, 0);
                    func_0804ad38("here is %s\n", s->func_0806d674());
                    return false;
                }
                /* fall through */
            case 3:
                if (!s->setOptTable(wt)) {
                    func_0804ad38("set opt table - failed \n");
                    return false;
                }
                break;
            default:
                func_0804ad38("bad alloc mode in dorawandsynth\n");
                return false;
            }
        }
        if (s->func_0806d908() == 4) {
            workTable *wt = s->getTable();
            if (s->getAllocMode() != 2) {
                func_0804ad38("flowMon::doLinear() - table hasn't had external allocation .. %s mode %d!\n",
                              s->func_0806d674(), s->getAllocMode());
                return false;
            }
            wt->unk_08 = 3;
        }
    }
    return true;
}

bool relaxOpt(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        switch (s->func_0806d908()) {
        case 3: {
            workTable *opt = s->unk_214;
            if (!opt) {
                func_0804ad38("relaxopt - no opt for %s\n", s->func_0806d674());
                return false;
            }
            workTable *wt = s->getTable();
            if (opt != wt) {
                func_0804ad38("relaxOpt - work and opt not same\n");
                return false;
            }
            wt->dump();
            for (Instr *i = s->getTable()->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
                i->unk_00 = 0;
                i->unk_08 = 0;
                i->unk_04 = 0;
            }
            s->unk_214 = 0;
            break;
        }
        case 4:
            break;
        case 1:
            func_0804ad38("uta found at relax time %s\n", s->func_0806d674());
            return false;
        default:
            func_0804ad38("unknown code mode found at relax time %s\n", s->func_0806d674());
            return false;
        }
    }
    return true;
}

bool func_08079fdc(Program *p, int smart, bool sizeReduce)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->func_0806d908() == 3) {
            if (!prepExtInfo(p, s, 0)) {
                func_0804ad38("failed to prep external info for stream\n");
                return false;
            }
            if (!optReset(p)) {
                func_0804ad38("failed to optReset cpu\n");
                func_0804adf0(-1);
            }
            workTable *wt = makeForceTable(p, s);
            if (!wt) {
                func_0804ad38("failed to prepUta istream\n");
                return false;
            }
            if (!func_0807e528(p, s, wt)) {
                func_0804ad38("failed to do initial reduction test\n");
                return false;
            }
            if (s->func_0806d9a0()) {
                if (!func_0807e740(p, wt, s->unk_261, s->unk_262)) {
                    func_0804ad38("failed to linear reduce table\n");
                    return false;
                }
                if (!sredTable(wt)) {
                    func_0804ad38("failed to prep for sred for %s\n", s->func_0806d674());
                    return false;
                }
            }
            if (!wt->doPostOpt(s->func_0806d968())) {
                wt->dump();
                func_0804ad38("failed to post opt after makeForce\n");
                return false;
            }
            if (!s->setOptTable(wt)) {
                func_0804ad38("set opt table - failed \n");
                return false;
            }
        }
    }
    return true;
}

const char *getOptimiserName(int id)
{
    int numOptimisers = 11;
    if (id < 0 || id >= numOptimisers) {
        func_0804ad38("asked to get an out of bounds optimiser id\n");
        func_0804aeb0();
    }
    return optTable[id].name;
}

extern const char sredEarliestAfterError[] = "int_Sred.. tline>iline..earlieset insert is after instruction.. mode is upwards %d>%d mfs=%d\n";
extern const char sredMoveInstError[] = "failed to move inst to tline1\n";
extern const char sredLatestBeforeError[] = "latest possible insert is before instruction..%d<%d!\n";
extern const char sredBadlyPlacedError[] = "badly placed instruction detected\n";
