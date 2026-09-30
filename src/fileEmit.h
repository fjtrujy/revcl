// Instruction text emission helpers (fileEmit.cpp, CIE group 40; hook in group 43).
#ifndef FILEEMIT_H
#define FILEEMIT_H

class Instr;

char *func_0808750c(char *buf, Instr *in, bool upper, char *mods, bool b, int zero);
char *emitDestMask(char *p, int mdBits, unsigned char mods);

#endif
