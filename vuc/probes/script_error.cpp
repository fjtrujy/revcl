// Experimental reconstruction of VUC script_error() diagnostics.
#include <iostream>
extern const char *currFileName;
extern int script_line;
extern bool error;

void script_error(const char *message)
{
    error = true;
    std::cout << currFileName << "(" << script_line << "):"
              << message << std::endl;
}
