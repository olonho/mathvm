#include "../../../../include/ast.h"

#pragma once


namespace mathvm {
    class AstBuilder {
        AstNode* visit(AstNode* node) {
            #define IFNODE(type, _) if (node->is##type()) return visit(node->as##type());
            FOR_NODES(IFNODE)
            #undef IFNODE
            return null;
        }

        #define VISITNODE(type, _) virtual AstNode* visit##type() { return NULL; };

        FOR_NODES(VISITNODE)

        #undef VISITNODE
    };
}