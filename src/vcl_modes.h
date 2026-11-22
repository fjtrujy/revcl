#ifndef VCL_MODES_H
#define VCL_MODES_H

// Recovered numeric protocol values. Keep the storage fields as byte/char:
// changing their types would change the historical i386 ABI and/or GCC code.
enum VclCodeMode {
    VCL_CODE_INVALID = 0,
    VCL_CODE_UTA = 1,
    VCL_CODE_MID = 2,
    VCL_CODE_RAW = 3,
    VCL_CODE_SYNTHETIC = 4,
    VCL_CODE_RAW_PREEXPANDED = 5,
    VCL_CODE_PASSTHROUGH = 6
};

// Operand descriptor/access direction; READ_WRITE is a combined bit mask.
enum VclOperandDirection {
    VCL_OPERAND_NONE = 0,
    VCL_OPERAND_READ = 1,
    VCL_OPERAND_WRITE = 2,
    VCL_OPERAND_READ_WRITE = 3
};

enum VclStreamStartKind {
    VCL_STREAM_START_NULL = 1,
    VCL_STREAM_START_ENTRY = 2
};

enum VclStreamEndKind {
    VCL_STREAM_END_NULL = 1,
    VCL_STREAM_END_CONTINUE = 2,
    VCL_STREAM_END_FINAL = 3,
    VCL_STREAM_END_MACRO = 4
};

// Raw table scheduling modes, as identified by rawModeName().
enum VclRawSchedulingMode {
    VCL_RAW_INVALID = 0,
    VCL_RAW_COMPILER_FREE = 1,
    VCL_RAW_COMPILER_FIXED = 2,
    VCL_RAW_USER_FREE = 3,
    VCL_RAW_USER_FIXED = 4
};

// Register allocation states, as identified by allocModeName().
enum VclAllocationMode {
    VCL_ALLOC_INVALID = 0,
    VCL_ALLOC_NONE = 1,
    VCL_ALLOC_X = 2,
    VCL_ALLOC_DONE = 3
};

// VU vector component mask. The register data remains byte-sized.
enum VclComponentMask {
    VCL_COMPONENT_X = 1,
    VCL_COMPONENT_Y = 2,
    VCL_COMPONENT_Z = 4,
    VCL_COMPONENT_W = 8,
    VCL_COMPONENT_XYZ = 7,
    VCL_COMPONENT_ALL = 15
};

// Operand kinds follow the opcode-descriptor table and opKindName().
enum VclOperandKind {
    VCL_OP_NULL = 0,
    VCL_OP_V_FL4 = 1,
    VCL_OP_V_INT = 2,
    VCL_OP_V_FL1 = 3,
    VCL_OP_V_I = 4,
    VCL_OP_V_P = 5,
    VCL_OP_V_NP = 6,
    VCL_OP_V_Q = 7,
    VCL_OP_V_R = 8,
    VCL_OP_V_ACC = 9,
    VCL_OP_V_ACCXYZ = 10,
    VCL_OP_V_FLXYZ = 11,
    VCL_OP_V_FLX = 13,
    VCL_OP_V_FLY = 14,
    VCL_OP_V_FLZ = 15,
    VCL_OP_V_FLW = 16,
    VCL_OP_V_CLIP = 17,
    VCL_OP_V_STAT_ALU_SHADOW = 18,
    VCL_OP_V_STAT_DIV_SHADOW = 19,
    VCL_OP_V_STAT_DIV_REGISTER = 20,
    VCL_OP_V_STAT_ALU_REGISTER = 21,
    VCL_OP_V_STAT_DIV = 22,
    VCL_OP_V_STAT_ALU = 23,
    VCL_OP_V_STAT_TST = 24,
    VCL_OP_V_STAT_SET = 25,
    VCL_OP_V_MAC = 26,
    VCL_OP_IMM = 27,
    VCL_OP_V_EA = 28,
    VCL_OP_V_EA_IMM = 29,
    VCL_OP_V_EA_REG = 30,
    VCL_OP_V_EA_IMM_REG = 31,
    VCL_OP_V_EA_INC = 32,
    VCL_OP_V_EA_DEC = 33,
    VCL_OP_V_INT1 = 34,
    VCL_OP_V_FL4S = 35,
    VCL_OP_V_INTS = 36,
    VCL_OP_V_MEMREG = 37,
    VCL_OP_V_TBD = 38,
    VCL_OP_C_GPR32 = 39,
    VCL_OP_C_GPR64 = 40,
    VCL_OP_C_GPR128 = 41,
    VCL_OP_C_GPRS = 42,
    VCL_OP_C_FPR = 43,
    VCL_OP_C_FPRS = 44,
    VCL_OP_V_CLIPA = 45,
    VCL_OP_C_EA_IMM = 46,
    VCL_OP_C_HILO0 = 47,
    VCL_OP_C_HILO1 = 48,
    VCL_OP_C_FSTAT = 49,
    VCL_OP_C_FCOND = 50,
    VCL_OP_C_FACC = 51,
    VCL_OP_C_FDIV = 52,
    VCL_OP_C_SHFTA = 53,
    VCL_OP_C_GPR64_31_RA = 54,
    VCL_OP_EXT_DEP = 55,
};

// Low byte of InstructionStream's packed stall reason. Resource slot 0..8
// is encoded in the high byte when the low byte is RESOURCE_PRESSURE.
enum VclStallReason {
    VCL_STALL_NONE = 0,
    VCL_STALL_INSTRUCTION_LATENCY = 1,
    VCL_STALL_INSTRUCTION_THROUGHPUT = 2,
    VCL_STALL_UPPER_PIPE_PRESSURE = 3,
    VCL_STALL_LOWER_PIPE_PRESSURE = 4,
    VCL_STALL_CANNOT_LOOP = 5,
    VCL_STALL_DEPENDENCY_CHAIN_DEPTH = 6,
    VCL_STALL_DEPENDENCY_CHAIN_HEIGHT = 7,
    VCL_STALL_BRANCH_CHAIN_HEIGHT = 8,
    VCL_STALL_RESOURCE_PRESSURE = 9,
    VCL_STALL_CIRCULAR_DEPENDENCY = 10
};

enum {
    VCL_STALL_REASON_MASK = 0xff,
    VCL_STALL_RESOURCE_SHIFT = 8
};

#endif
