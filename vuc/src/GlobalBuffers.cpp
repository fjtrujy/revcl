// Source-level reconstruction of VUC's original statically allocated text
// and assembly buffers. These are real ELF STT_OBJECT names and sizes.
// The linker addresses are never embedded in candidate source.
char line[1024] = {};
char word[1024] = {};
char outputFileName[256] = {};
char inputPath[256] = {};
char asmBuffer[65536] = {};
