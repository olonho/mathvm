//
// Created by andy on 11/13/16.
//

#ifndef PROJECT_TYPEDEDUCER_H
#define PROJECT_TYPEDEDUCER_H

#include <unordered_map>
#include "visitors.h"

namespace mathvm {
class TypeDeducer : public AstBaseVisitor
{
    std::unordered_map<AstNode*, VarType> _nodeType;
    Code *code;
public:
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    VarType getNodeType(AstNode *node);
    TypeDeducer(Code *code) : code(code) {}
};
}

#endif //PROJECT_TYPEDEDUCER_H
