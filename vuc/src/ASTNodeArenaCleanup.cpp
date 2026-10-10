// Byte-exact historical AST arena static object teardown.
#include <iostream>
#include <new>
class ASTNode { public: static char *mem; };
char *ASTNode::mem;
struct ArenaCleanup {
    ArenaCleanup() {
        char *chunk = (char *)::operator new(0x10008);
        *(char **)chunk = chunk + 4;
        *(char **)(chunk + 0x10004) = 0;
        ASTNode::mem = chunk;
    }
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
