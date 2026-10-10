// Byte-exact historical AST arena static object teardown.
#include <iostream>
#include <new>
class ASTNode { public: static char *mem; };
char *ASTNode::mem;
struct ArenaCleanup {
    ~ArenaCleanup() {
        while (ASTNode::mem) {
            char *current = ASTNode::mem;
            char *next = *(char **)(current + 0x10004);
            ::operator delete(current);
            ASTNode::mem = next;
        }
    }
};
static ArenaCleanup cleanup;
