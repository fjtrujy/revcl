// Exact original VUC global and local hash table cleanup callbacks.
#include <iostream>
#include "MLHashTable.h"
#include "MLHashFunGameString.h"
MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> globalEnv;
MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> localEnv;
