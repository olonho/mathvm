#pragma once
#include <ostream>
#include <ast.h>

class ShowVisitor: public mathvm::AstVisitor {
    bool need_tabs;
    int prec;
    int level;
    std::ostream& stream;
    const int tabs;
public:
    ShowVisitor(std::ostream& o, int tabs_ = 4);
    void show(mathvm::AstNode* node) { node->visit(this); }
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};
