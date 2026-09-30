// VU component-mask string helpers ("xyzw" <-> bit mask), CIE group 3.
#ifndef VUSTR_H
#define VUSTR_H

bool func_08051f08(const char *s, int *mask, bool *bad);
bool func_08051f78(const char *s, int *mask);
int func_08051ff0(const char *s);
char *func_08052064(int mask);
char *func_080520d4(int mask);
int func_08052100(const char *s);
int func_0805219c(const char *s);
int func_080521f0(char *s);
bool func_080522c4(int mods, unsigned char mode, bool flag);
char *func_08052314(int mods);

#endif
