#pragma once

#include "visitors.h"

namespace mathvm 
{

struct ast_printer 
    : AstBaseVisitor 
{
    ast_printer();
    ~ast_printer();

    string print_tree(AstNode *head);

#define VISITOR_FUNCTION(type, name) void visit##type(type * node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void indent();
    static bool is_top(FunctionNode const *node);

private:
    stringstream stream_;
    
    int indent_;
};

} // namespace mathvm 