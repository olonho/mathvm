#ifndef _MATHVM_AST_PRINTER_H
#define _MATHVM_AST_PRINTER_H

#include "visitors.h"
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class AstPrinter : public AstVisitor
{
    std::ostream* output_;

    void functionDeclarations( Scope* scope );
    void variableDeclarations( Scope* scope );
    void blockContents( BlockNode* node );

public:
    AstPrinter();

    void print(AstFunction* root, std::ostream& output = std::cout) {
        output_ = &output;
        if (root) {
            blockContents(root->node()->body());
        }
    }

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif // _MATHVM_AST_PRINTER_H