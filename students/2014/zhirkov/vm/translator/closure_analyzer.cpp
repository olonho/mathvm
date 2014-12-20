#include "closure_analyzer.h"

namespace mathvm {
    void ClosureAnalyzer::visitAstFunction(AstFunction const* f) {
        if  (_functionStack.empty())
            _result->functions[f] = new ClosureInfo::Function(NULL);
        else
            _result->functions[f] = new ClosureInfo::Function(_functionStack.top());
        _functionStack.push(f);
        _result->scopesToFunctions[f->scope()] = f;
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

    void ClosureAnalyzer::debug() {
        _debug << "Closure analyser: \n functions: \n";
        for (auto f: _result->functions) {
            _debug << f.first->name() << " : \n   captured locals: ";
            for (auto v : f.second->capturedLocals) _debug << v->name() << ' ';
            _debug << "\n   captured from outside: ";
            for (auto v : f.second->capturedOuterVars) _debug << v->name() << ' ';
            _debug << std::endl;
        }

        _debug << "-----\n  vars: " << std::endl;
        for (auto m : _result->vars) if (! m.second.capturedBy.empty()){
            _debug << m.first->name() << " is ";
            if (m.second.isRead) _debug << " read ";
            if (m.second.isWritten) _debug << " written ";
            _debug << " inside: ";

            for (auto f : m.second.capturedBy)
                _debug << f->name() << ' ';
            _debug << std::endl;
        }
        _debug << "-----" << std::endl;
    }

    void ClosureAnalyzer::declareFunction(AstFunction const *fun) {
    }
}