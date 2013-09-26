#ifndef PRINTER_VISITOR
#define PRINTER_VISITOR

#include <ostream>

#include "mathvm.h"
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class Printer : public AstVisitor {
public:
    explicit Printer(std::ostream& out = std::cout): 
        level(0), out(out), next_line_after_block(true) {
    }

    void print(AstFunction* root) {
        if (root) {
            printBlockContents(root->node()->body());
        }
    }

#define VISITOR_FUNCTION_DECLARATION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION_DECLARATION)
#undef VISITOR_FUNCTION_DECLARATION

private:
    void printBlockContents(mathvm::BlockNode* node);

    int level;
    std::ostream& out;
    int next_line_after_block;

    void indent() {
        for (int i = 0; i < level; i++)
            out << "    ";
    }
};

}

#endif
