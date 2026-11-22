#ifndef IGROUPLOG_H
#define IGROUPLOG_H

class Instr;
class workTable;

// "overflow for the iGroup log"
class iGroupLog {
public:
    static Instr **table;
    static int count;
    static int size;

    iGroupLog();
    ~iGroupLog();
    static int init();
    static int reset();
    static int add(Instr *ins);
    static int logTable(workTable *sourceTable);
};

#endif
