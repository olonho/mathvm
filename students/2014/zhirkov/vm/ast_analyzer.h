#pragma once

#include "../../../../include/ast.h"

namespace mathvm {
    struct AstAnalyzerContext {
        AstAnalyzerContext() : scope(NULL) {
        }

        Scope* scope;
    };

    template<typename T, typename Ctx>
    class AstAnalyzer: public AstVisitor {
    protected:
        T* _result;
        AstFunction const* const top;
        Ctx ctx;
    public:

        AstAnalyzer(AstFunction const* top) : top(top), _result(new T()) {
        }

        virtual void start()  { visitAstFunction(top); };
        T* getResult() {
            return _result;
        }
        virtual void visitAstFunction(AstFunction const* fun) = 0;


#define DEFAULT_VISITOR(type, _) virtual void visit##type(type* node) { node->visitChildren(this); }
        FOR_NODES(DEFAULT_VISITOR)

    };


}