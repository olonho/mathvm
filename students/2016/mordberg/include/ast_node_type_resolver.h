#pragma once

#include <mathvm.h>

#include <map>

namespace mathvm {

namespace type_resolver {

class AstNodeTypeResolver {

public:

    AstNodeTypeResolver(AstFunction* astTop);

    VarType getNodeType(AstNode* node) const {
        if (_resolver.count(node)) {
            return _resolver[node];
        }
        return VT_INVALID;
    }

private:
    std::map<AstNode*, VarType> _resolver;
};

} // namespace type_resolver

} // namespace mathvm