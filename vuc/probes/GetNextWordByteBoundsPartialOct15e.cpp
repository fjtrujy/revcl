// EXPERIMENTAL VUC 2004.1 GetNextWord original-sized C++03 reconstruction.
// 23/135 original-offset equal machine-code bytes, 5 genuine global-variable
// relocations; Jan25 and Dec06 GCC 3.3.3 + GNU as 2.15 verified.
// Does not establish byte-exact code or original source authorship.
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
unsigned char c=*input;
 if(c==0||c==9||c==32||c==44||c==46||c==91)goto endword;
 *dest++=c;
 ++input;
 ++position;
 currentPosition=position;
 }
endword:
 *dest=0;
 return input;
}
