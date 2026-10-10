// Experimental global initialization wrapper for the currFileName TU.
// Its helper has been accepted independently, but the 31-byte global wrapper
// still does not match; keep this as a non-accepted comparison probe.
#include <iostream>
char *currFileName = (char *)"";
bool error;
struct ErrorReset {
    ErrorReset() { error = false; }
};
static ErrorReset resetError;
