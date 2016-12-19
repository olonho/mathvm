#pragma once

#include "mathvm.h"
#include "ast.h"

#include <map>
#include <stack>

namespace mathvm {

namespace type_resolver {

class AstNodeTypeResolver {

public:

    AstNodeTypeResolver(AstFunction* astTop);

    VarType operator() (AstNode* node) const;

private:
    std::map<AstNode*, VarType> _resolver;
    std::stack<Scope*> _scopes;
};

} // namespace type_resolver

} // namespace mathvm