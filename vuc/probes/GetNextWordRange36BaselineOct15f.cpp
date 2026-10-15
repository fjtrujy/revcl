// Immutable experimental 36/135 C++03 scanner source baseline for
// reproducible source/assembler topology sweeps, not the selected result.
// Original-length 135 bytes with 5 real relocations under GCC 3.3.3.
extern int currentPosition;
extern int wordPosition;
extern char word[1024];
const char *GetNextWord(const char *input) {
int position=currentPosition;
for (;;) {
 const unsigned char c = *input;
 if (c <= 10) { if (c < 9) break; }
 else if (c != 32) break;
 ++position; ++input; currentPosition=position;
}
 wordPosition=position;
 char *dest=word;
 for (;;) {
char c=*input;
if(c==44)goto endword;
if(c>44) {
 if(c==46)goto endword;
 if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;} else if(c==0)goto endword;
} *dest++=c;
 ++input;
 ++position;
 currentPosition=position;
 }
endword:
 *dest=0;
 return input;
}
