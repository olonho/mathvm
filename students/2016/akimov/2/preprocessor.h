#pragma once

#include <mathvm.h>
#include <visitors.h>

#include <map>
#include <stack>

namespace mathvm {

class Preprocessor : public AstVisitor {
public:
    Preprocessor() {}

    VarType getType(AstNode* node);

private:
    map<AstNode*, VarType> _types;
    stack<Scope*> _scopes;
    stack<FunctionNode*> _functions;

    void setType(AstNode* node, VarType type);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

struct TypeError : public ErrorInfoHolder {};

}
