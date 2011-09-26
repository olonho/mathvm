#pragma once
#include <ostream>
#include <ast.h>

class ShowVisitor: public mathvm::AstVisitor {
    bool need_sw;
    int prec;
    int level;
    std::ostream& stream;
public:
    ShowVisitor(std::ostream& o);
    void show(mathvm::AstNode* node) { node->visit(this); }
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
