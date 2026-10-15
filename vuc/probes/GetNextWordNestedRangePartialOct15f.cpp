// EXPERIMENTAL VUC 2004.1 GetNextWord original-length C++03.
// 50/135 identical original-offset instruction bytes; 5 genuine relocation
// sites, GCC 3.3.3 Jan25/Dec06 + GNU as 2.15 independently verified.
// Signed-char nested delimiter dispatch; no inline assembly or patching.
// This is NOT a byte-exact reconstruction of the full method.
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
if(c<=44) {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
} else {
 if(c==46)goto endword;
 if(c==91)goto endword;
} *dest++=c;
 ++input;
 ++position;
 currentPosition=position;
 }
endword:
 *dest=0;
 return input;
}
