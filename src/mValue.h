// Small typed value: int tag + owned string (class name guessed).
#ifndef MVALUE_H
#define MVALUE_H

class mValue {
public:
    int type;  // 4 = owns a new[] string
    char *str;

    mValue();
    ~mValue();
};

#endif
