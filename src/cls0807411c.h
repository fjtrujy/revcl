// Table-like class (defined in another unit); placeholder name.
#ifndef CLS0807411C_H
#define CLS0807411C_H

class cls0807411c {
public:
    char unk_00[9];
    unsigned char unk_09;   // table kind: 2,3,5 = raw/raw_preexp/mid
    char unk_0a[0x22];
    void *unk_2c;           // table
    int unk_30;             // range start
    int unk_34;             // range end
    int unk_38;             // lsz

    int func_0807411c();
    int func_080741b4();
    int func_080741c0();
};

#endif
