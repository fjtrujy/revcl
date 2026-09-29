// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"

class workTable;
struct regUse;
class instStream;

struct linkNode {
    char unk_00[8];
    linkNode *unk_08;           // next
    instStream *unk_0c;         // other stream
    char unk_10;                // marked
    char unk_11[3];
    int unk_14;                 // link type
};

struct linkList {
    linkNode *unk_00;           // head
    char unk_04[8];
    linkNode *func_0806dff4(instStream *s, int type);
};

class instStream {
public:
    const char *func_0806d674();  // stream name or default
    workTable *getWorkTable()
    {
        if (unk_218)
            return unk_218;
        func_0804ad38("work table not set in %s\n", func_0806d674());
        func_0804adf0(-1);
        return unk_218;
    }

    bool func_0805e4bc();         // gather stats
    bool func_0806563c(bool a, bool b);  // build memory dependencies
    bool func_0806d5cc();         // flow reset
    unsigned char func_0806d908(); // code mode
    unsigned char func_0806d968(); // work table mode
    const char *func_0806d934();  // code mode name

    char unk_00[0x108];
    instStream *unk_108;         // next in Program::unk_a0
    instStream *unk_10c;         // previous stream (to-register branch)
    char unk_110[0x128 - 0x110];
    linkList unk_128;            // input links
    linkList unk_134;            // output links
    char unk_140[0x218 - 0x140];
    workTable *unk_218;  // work table
    char unk_21c[0x224 - 0x21c];
    short unk_224;
    unsigned char unk_226;      // visit mode (0 unvisited, 1 active, 2 visited, 3 end match, 4 dead end)
    char unk_227;
    char unk_228;               // hazard flag
    char unk_229[0x22c - 0x229];
    int unk_22c;                // dependency length used by back trace
    short unk_230;
    char unk_232[2];
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
    char unk_261[0x5f];
    short unk_2c0;     // stream id
    char unk_2c2[0x2cd - 0x2c2];
    char unk_2cd;      // code mode (2 adds 1000 to back trace dep length)
    char unk_2ce[0x2e0 - 0x2ce];
    int unk_2e0;       // dependency length
    int unk_2e4;

    int func_0805f284();
    int func_0805f29c();
    int getOrGuessDepLength();
    int func_0805f354();
    int getOrGuessLogicalLength();
};

#endif
