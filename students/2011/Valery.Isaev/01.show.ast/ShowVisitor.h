#ifndef _SHOW_VISITOR_H_
#define _SHOW_VISITOR_H_

#include <ostream>

#include "ast.h"

class ShowVisitor: public mathvm::AstVisitor {
    bool need_tabs;
    int prec;
    int level;
    std::ostream& stream;
public:
    ShowVisitor(std::ostream& o);
    void show(mathvm::AstFunction* node) { node->node()->visit(this); }
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#endif
