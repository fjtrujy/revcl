// Pointer-sized value used by the original i386 VCL implementation to hold
// addresses, tagged pointers, and pointer-derived handles in integer slots.
//
// Keep the historical 32-bit ABI (and its byte-exact compiler output) intact.
#ifndef VCL_PORTABILITY_H
#define VCL_PORTABILITY_H

#if defined(__APPLE__) && defined(__LP64__)
typedef long VclPtrWord;
typedef unsigned long VclUPtrWord;
#else
typedef int VclPtrWord;
typedef unsigned int VclUPtrWord;
#endif

#endif
