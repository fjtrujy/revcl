#ifndef BLOCK_H
#define BLOCK_H

class Instr;

#if defined(__APPLE__) && defined(__LP64__)
// The original Block is a decompiled overlay of workTable: it directly
// accesses flattened fields by their 32-bit x86 offsets. On macOS/LP64,
// recreate the *native* workTable layout rather than retaining those fixed
// 32-bit byte offsets. No Block object is ever constructed; these pointers
// reference actual workTable objects.
#include "workTable.h"
#include <stddef.h>

class Block {
public:
    int unk_00;
    short unk_04;
    short unk_06;
    char unk_08;
    unsigned char unk_09;
    unsigned char unk_0a;
    char unk_0b;
    int unk_0c;
    instStream *unk_10;
    regUseList unk_14;
    int unk_20;
    cls0805c244 *unk_24;
    int unk_28;
    wtRow *unk_2c;
    int unk_30;
    int unk_34;
    int unk_38;
    int unk_3c;
    int unk_40;
    int unk_44;
    char unk_48[4];
    Instr *unk_4c;
    int unk_50;
    cls08058718 unk_54;
    cls0805e2e4 unk_5c;
    short unk_74;
    short unk_76;
    short unk_78;
    short unk_7a;
    short unk_7c;
    char unk_7e[2];
    short unk_80;
    short unk_82;
    Instr *unk_84;
    Instr *unk_88;  // workTable::unk_88.unk_00
    Instr *unk_8c;  // workTable::unk_88.unk_04
    int unk_90;     // workTable::unk_88.unk_08
    Instr *unk_94;  // workTable::unk_94.unk_00
    Instr *unk_98;  // workTable::unk_94.unk_04
    int unk_9c;     // workTable::unk_94.unk_08
    Instr *unk_a0;  // workTable::unk_a0.unk_00
    Instr *unk_a4;  // workTable::unk_a0.unk_04
    int unk_a8;     // workTable::unk_a0.unk_08
    // The embedded wtInstrList rounds up to pointer alignment, whereas the
    // next Block field is only a char. Mirror its trailing padding as well.
    char wt_list_tail_padding[sizeof(wtInstrList) - 2 * sizeof(Instr *) - sizeof(int)];
    char unk_ac;
    char unk_ad;
    char unk_ae[2];
    int unk_b0;
    int unk_b4;
    int unk_b8;
    int unk_bc;
    int unk_c0[3];
};

// Fail compilation rather than silently corrupt instruction lists if a
// native structure definition changes without updating this overlay.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
#define VCL_BLOCK_OFFSET_CHECK(blockField, tableField) \
    typedef char vcl_block_offset_check_##blockField[ \
        offsetof(Block, blockField) == offsetof(workTable, tableField) ? 1 : -1]
VCL_BLOCK_OFFSET_CHECK(unk_38, unk_38);
VCL_BLOCK_OFFSET_CHECK(unk_4c, unk_4c);
VCL_BLOCK_OFFSET_CHECK(unk_50, unk_50);
VCL_BLOCK_OFFSET_CHECK(unk_88, unk_88);
VCL_BLOCK_OFFSET_CHECK(unk_94, unk_94);
VCL_BLOCK_OFFSET_CHECK(unk_a0, unk_a0);
VCL_BLOCK_OFFSET_CHECK(unk_ac, unk_ac);
VCL_BLOCK_OFFSET_CHECK(unk_b0, unk_b0);
VCL_BLOCK_OFFSET_CHECK(unk_c0, unk_c0);
#undef VCL_BLOCK_OFFSET_CHECK
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#else
class Block {
public:
    char unk_00[4];
    short unk_04;   // current height
    short unk_06;
    char unk_08[0x30];
    int unk_38;     // range width limit
    char unk_3c[0x10];
    Instr *unk_4c;  // move list head
    int unk_50;     // move list count
    char unk_54[0x88 - 0x54];
    Instr *unk_88;  // linear list tail
    Instr *unk_8c;  // linear list head
    int unk_90;     // linear list count
    Instr *unk_94;  // ToDo list head
    Instr *unk_98;  // ToDo list tail
    int unk_9c;     // ToDo list count
    Instr *unk_a0;  // Done list head
    Instr *unk_a4;  // Done list tail
    int unk_a8;     // Done list count
    char unk_ac;
    bool unk_ad;    // move limit reached
    int unk_b0;
    int unk_b4;
    int unk_b8;     // class-8 move count
    int unk_bc;     // count of instrs using both pipes
    int unk_c0[3];  // per-pipe instr counts, [2] = total
};
#endif

#endif
