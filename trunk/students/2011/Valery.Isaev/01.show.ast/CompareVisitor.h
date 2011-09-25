#ifndef _COMPARE_VISITOR_H_
#define _COMPARE_VISITOR_H_

#include <ostream>

#include "ast.h"

class CompareVisitor: public mathvm::AstVisitor {
    bool res;
    mathvm::AstNode* another;
public:
    CompareVisitor(mathvm::AstNode* o = 0);
    bool compare(mathvm::AstNode* node, mathvm::AstNode* o = 0);
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#endif
