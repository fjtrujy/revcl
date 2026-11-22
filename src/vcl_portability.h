// Keep pointer-derived integer handles wide enough on every architecture.
// intptr_t is 32-bit on the historical i386 ABI (preserving the original
// instructions) and 64-bit on modern LP64 Linux/macOS builds.
#ifndef VCL_PORTABILITY_H
#define VCL_PORTABILITY_H

#include <stdint.h>

typedef intptr_t VclPtrWord;
typedef uintptr_t VclUPtrWord;

// C++98-compatible static assertions (also supported by GCC 3.2.2).
typedef char VclPtrWordMustFitPointer[sizeof(VclPtrWord) == sizeof(void *) ? 1 : -1];
typedef char VclUPtrWordMustFitPointer[sizeof(VclUPtrWord) == sizeof(void *) ? 1 : -1];

#endif
