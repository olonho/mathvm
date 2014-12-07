#pragma once

#include "../../../../include/ast.h"
#include "ast_metadata.h"

namespace mathvm {
    inline AstMetadata &getData(AstNode *node) {
        if (! node->info())
            node->setInfo(new AstMetadata());
        return *((AstMetadata*)(node->info()));
    }

    inline void clearData(AstNode *node) {
        delete &getData(node);
        node->setInfo(NULL);
    }
}