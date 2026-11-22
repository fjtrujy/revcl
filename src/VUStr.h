// VU component-mask string helpers ("xyzw" <-> bit mask), CIE group 3.
#ifndef VUSTR_H
#define VUSTR_H

bool parseTrailingComponentMask(const char *s, int *mask, bool *bad);
bool parseBracketedComponentMask(const char *s, int *mask);
int decodeComponentMaskLetters(const char *s);
char *formatComponentMaskLetters(int mask);
char *formatDottedComponentMask(int mask);
int readTrailingComponentMask(const char *s);
int lastComponentBit(const char *s);
int removeInstructionModifierGroups(char *s);
bool areInstructionModifiersAllowed(int mods, unsigned char mode, bool flag);
char *formatInstructionModifiers(int mods);

#endif
