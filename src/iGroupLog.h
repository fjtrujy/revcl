#ifndef IGROUPLOG_H
#define IGROUPLOG_H

class Instr;

// "overflow for the iGroup log"
class iGroupLog {
public:
    static Instr **table;
    static int count;
    static int size;

    iGroupLog();
    ~iGroupLog();
};

#endif
