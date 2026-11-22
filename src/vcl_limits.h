#ifndef VCL_LIMITS_H
#define VCL_LIMITS_H

// Compile-time capacities of the recovered fixed-size storage.
// Keep these as C++98 integer constants: no extra storage or symbols.
enum VclFixedCapacity {
    VCL_REGISTER_WEB_CAPACITY = 0x4000,
    VCL_IMPLICIT_OPERAND_TEXT_CAPACITY = 0x800,
    VCL_TOKEN_COPY_LIMIT = 0x800,
    VCL_MAX_DECODED_SOURCE_LINES = 0x1000,
    VCL_MIN_PHYSICAL_INSTRUCTIONS = 64,
    VCL_MAX_PHYSICAL_INSTRUCTIONS = 0x4000
};

#endif
