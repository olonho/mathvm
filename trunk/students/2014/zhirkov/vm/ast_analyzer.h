#pragma once

#include "../../../../include/ast.h"

namespace mathvm {
    template<typename T>
    class AstAnalyzer: public AstVisitor {
    protected:
        T _result;
        AstNode const &startNode;

    public:

        AstAnalyzer(AstNode const &startNode) : startNode(startNode) {
        }

        T getResult() {
            return _result;
        }

        #define DEFAULT_VISITOR(type, _) virtual void visit##type(type* node) { node->visitChildren(this); }
        FOR_NODES(DEFAULT_VISITOR)

    };


}