#include "closure_analyzer.h"

namespace mathvm {
    void ClosureAnalyzer::visitAstFunction(AstFunction const* f) {
        _functionStack.push(f);
        embraceVars(f->scope());
        f->node()->visit(this);
        _functionStack.pop();
    }
    void ClosureAnalyzer::visitLoadNode(LoadNode *node) {
        if (!isFunctionLocal(node->var()))
            captureRead(node->var());
        node->visitChildren(this);
    }

    void ClosureAnalyzer::visitBlockNode(BlockNode *node) {
        _scope = node->scope();
        embraceVars(node->scope());
        node->visitChildren(this);
        Scope::FunctionIterator fit(node->scope());
        while( fit.hasNext())
            visitAstFunction(fit.next());

        _scope = node->scope()->parent();
    }

    void ClosureAnalyzer::visitStoreNode(StoreNode *node) {
        if (!isFunctionLocal(node->var()))
                captureWrite(node->var());
        node->visitChildren(this);
    }

}