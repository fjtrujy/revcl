// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"
#include "mList.h"
#include "workTable.h"
#include "Instr.h"
#include "statCounts.h"
#include "linkList.h"

class workTable;
class Instr;
struct regUse;
class instStream;
struct Program;

// 16-byte record in the leading table of a stream (name guessed).
struct istRec {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
};

class instStream {
public:
    instStream();
    const char *func_0806d674();  // stream name or default
    workTable *getWorkTable()
    {
        if (unk_218)
            return unk_218;
        func_0804ad38("work table not set in %s\n", func_0806d674());
        func_0804adf0(-1);
        return unk_218;
    }
    // Same check as getWorkTable, written as a guarded block (different code shape).
    workTable *getTable()
    {
        if (!unk_218) {
            func_0804ad38("work table not set in %s\n", func_0806d674());
            func_0804adf0(-1);
        }
        return unk_218;
    }

    workTable *checkedTable(const char *msg)
    {
        if (unk_218)
            return unk_218;
        func_0804ad38(msg);
        func_0804adf0(-1);
        return unk_218;
    }
    void requireTable(const char *msg)
    {
        if (!unk_218) {
            func_0804ad38(msg);
            func_0804adf0(-1);
        }
    }

    bool func_0805e4bc();         // gather stats
    bool func_0806563c(bool a, bool b);  // build memory dependencies
    bool flowReset();
    const char *branchTypeName();
    const char *endTypeName();
    const char *startTypeName();
    char *emit(char *buf, int a, int *p1, int *p2);
    bool checkStartEnd()
    {
        int st = unk_2cc;
        int et = unk_2cd;
        bool ok = false;
        if (st == 1 || st == 2)
            ok = et >= 1 && et <= 4;
        return ok;
    }
    unsigned char func_0806d908(); // code mode
    unsigned char func_0806d968(); // work table mode
    const char *getCodeModeName();
    void dumpSynthetics();
    char *emitLabels(char *buf);
    Instr *getBranch();
    bool removeBranch();
    char *getBranchTargetImmediate();
    RegPoolEntry *getBranchTargetRegister();
    ~instStream();
    bool reset();
    void dump(int);
    void zeroStats();
    bool iOwn(Instr *i);
    bool patchLatencies();
    bool canLoop();
    bool func_0806d9a0();         // raw-mode work table
    bool func_0806d9d4();
    unsigned char getAllocMode();
    bool func_0806da34();         // not raw mode
    bool func_0806da58();
    bool func_0806da80();         // out-of-line checkStartEnd
    bool disposeInputTable();
    bool processCmdSynth(Instr *i);
    void func_0806dc9c(instStream *src);  // copy record table from src

    int unk_00;                  // record count
    istRec unk_04[16];
    Program *unk_104;
    instStream *unk_108;         // next in Program::unk_a0
    instStream *unk_10c;         // previous stream (to-register branch)
    mList unk_110;               // name list
    char *unk_124;               // pass thru
    linkList unk_128;            // input links
    linkList unk_134;            // output links
    char unk_140;
    char unk_141[3];
    int unk_144;
    workTable unk_148;           // own work table
    int unk_214;
    workTable *unk_218;  // work table
    workTable *unk_21c;  // tidied work table
    int unk_220;
    short unk_224;
    unsigned char unk_226;      // visit mode (0 unvisited, 1 active, 2 visited, 3 end match, 4 dead end)
    char unk_227;
    char unk_228;               // hazard flag
    char unk_229[0x22c - 0x229];
    int unk_22c;                // dependency length used by back trace
    short unk_230;
    short unk_232;
    regUse *unk_234;
    unsigned *unk_238;          // m_tStore
    int unk_23c;                // dst key
    int unk_240;
    instStream *unk_244;        // min-path predecessor
    int unk_248;
    instStream *unk_24c;        // max-path predecessor
    instStream *unk_250;        // next in Program::unk_dc
    instStream *unk_254;        // next in Program::unk_d8
    instStream *unk_258;        // next in Program::unk_e0
    instStream *unk_25c;        // next in Program::unk_e4
    char unk_260;      // set when stream takes part in a stall
    char unk_261;
    char unk_262;
    char unk_263;
    char unk_264;      // reached top (accessible)
    char unk_265;      // collapse into successor (tidy)
    char unk_266[2];
    int unk_268;
    int unk_26c;
    int unk_270;                 // known min count
    int unk_274;                 // minimum safe count
    cls0805e2e4 unk_278;
    statCounts unk_290;
    short unk_2b4;
    short unk_2b6;
    short unk_2b8;
    short unk_2ba;
    int unk_2bc;
    short unk_2c0;     // stream id
    short unk_2c2;     // stream serial
    short unk_2c4;
    short unk_2c6;
    int unk_2c8;
    unsigned char unk_2cc;  // start type (ISTST_*)
    unsigned char unk_2cd;  // end type (ISTET_*; 2 adds 1000 to back trace dep length)
    unsigned char unk_2ce;  // branch type
    char unk_2cf;
    char *unk_2d0;
    char unk_2d4;      // dbgTop
    char unk_2d5;      // dbgEnd
    char unk_2d6;
    char unk_2d7;
    int unk_2d8;
    char unk_2dc;
    char unk_2dd[3];
    int unk_2e0;       // dependency length
    int unk_2e4;

    int func_0805f284();
    int func_0805f29c();
    int getOrGuessDepLength();
    int func_0805f354();
    int getOrGuessLogicalLength();
};

#endif
