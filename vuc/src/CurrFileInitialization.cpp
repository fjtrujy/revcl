// Byte-exact VUC parser static initialization helper, not its global wrapper.
// The source type/declaration names remain original-ABI hypotheses.
#include <iostream>
char *currFileName = (char *)"";
bool error;
struct ErrorReset {
    ErrorReset() { error = false; }
};
static ErrorReset resetError;
