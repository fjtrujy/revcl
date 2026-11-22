#include "Instr.h"
#include "decodedLine.h"
#include "fileEmit.h"
#include "Processor.h"
char *emitReversibleBranch(char *buf, Instr *in, bool upper, char *mods, bool b, int);
bool extHook(void *, Instr *in);
bool rotateInputComponentMask(void *, Instr *in);
bool efu_Hook(void *p, Instr *in);
bool wr2w_Hook(void *p, Instr *in);
char *emitClip(char *buf, Instr *in, bool upper, char *mods, bool b, int);
char *vmove_Emit(char *buf, Instr *in, bool upper, char *mods, bool b, int);
bool semSynth(void *p, Instr *in);

const char *pipeNames[16] = {
    "VPU_FMAC", "VPU_IALU", "VPU_MOVE", "VPU_LS", "VPU_EFU",
    "VPU_DIV", "VPU_RAND", "VPU_EXT", "VPU_BRANCH", "0",
};

RegDesc regDescTable[] = {
#include "regDescTable.h"
};

OpcodeDescriptor vuOpcodeDescriptors[] = {
#include "vuITable.h"
};

PoolDef vuRegisterPoolDefinitions[] = {
    {1, "vf", "VU User floating point", 9, 4, "vf00", 0},
    {1, "vi", "VU User integer", 8, 1, "vi00", 0},
    {2, "hw", "VU hardware register", 10, 0, 0, "EXT"},
    {0, 0, 0, 0, 0, 0, 0},
};

CpuDef vuProcessorDefinition = {
    "Marigold",
    2,
    vuRegisterPoolDefinitions,
    pipeNames,
    vuOpcodeDescriptors,
    pairedOperandDescriptions,
    operandCountOverrides,
};

