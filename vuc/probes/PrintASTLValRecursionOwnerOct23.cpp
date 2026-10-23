// First source-directed reconstruction of original Print(ASTLValExpr*) variants.
// Original ELF 0x0805c4f0: kind +0x1c, payload +0x24/+0x28,
// three case-specific pushes to matchStack, with exception-safe pops.
// This is an experimental incomplete candidate, not accepted byte-exact C++.
#include "../src/MLGameString.h"
#include <ostream>
#include <new>
#include <cstring>
// RTTI from the original ELF authenticates ASTLValExpr -> ASTExpr -> ASTNode.
// The original Print(ASTExpr*) dispatcher reads kind at +8; the LVal printer
// reads a separate discriminator at +0x1c, after four unknown ASTExpr words.
struct ASTNode { virtual void apa(); int line; };
struct ASTExpr : ASTNode {
    int kind;
    unsigned originalExprFields[4];
};
struct ASTLValExpr : ASTExpr {
    int lvalueKind;
    unsigned originalVariantField;
};
struct ASTLValIDExpr : ASTLValExpr {
    MLGameString name;
};
struct ASTLValMemExpr : ASTLValExpr {
    ASTLValExpr *base;
    ASTLValExpr *member;
};
struct ASTLValArrayExpr : ASTLValExpr {
    ASTLValExpr *base;
    ASTExpr *index;
};
struct MatchStack {
    int length;
    int capacity;
    // Shared by Print(ASTStmt*), Print(ASTExpr*), Print(ASTDecl*) and this
    // printer. Each entry must therefore own a common ASTNode*, not LVal*.
    ASTNode **entries;
    void Add(ASTNode *node) {
        int previous = length;
        if (previous >= capacity) {
            int expanded = capacity ? 2 * capacity : 16;
            ASTNode **oldEntries = entries;
            entries = new ASTNode *[expanded];
            if (oldEntries)
                __builtin_memcpy(entries, oldEntries, previous * sizeof(ASTNode *));
            capacity = expanded;
        }
        entries[previous] = node;
        length = previous + 1;
    }
    void Pop() {
        if (length > 0)
            --length;
    }
};
extern MatchStack matchStack;
class ScopedPrintNode {
public:
    ScopedPrintNode(ASTLValExpr *node) { matchStack.Add(node); }
    ~ScopedPrintNode() { matchStack.Pop(); }
};
void Print(ASTExpr *, std::ostream &);
void Print(ASTLValExpr *, std::ostream &);
void Print(ASTLValExpr *node, std::ostream &output)
{
    if (!node)
        return;
    switch (node->lvalueKind) {
    case 0: {
        ScopedPrintNode entry(node);
        output << static_cast<ASTLValIDExpr *>(node)->name.c_str();
        break;
    }
    case 1: {
        ScopedPrintNode entry(node);
        ASTLValArrayExpr *array = static_cast<ASTLValArrayExpr *>(node);
        Print(array->base, output);
        output << "[";
        Print(array->index, output);
        output << "]";
        break;
    }
    case 2: {
        ScopedPrintNode entry(node);
        ASTLValMemExpr *member = static_cast<ASTLValMemExpr *>(node);
        Print(member->base, output);
        output << ".";
        Print(member->member, output);
        break;
    }
    }
}
