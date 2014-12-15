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
        std::ostream& _debug;
    public:

        AstAnalyzer(AstFunction const* const top, std::ostream& debugStream) : top(top), _result(new T()), _debug(debugStream) {
        }

        virtual void start()  { visitAstFunction(top); };
        T* getResult() const {
            return _result;
        }
        virtual void visitAstFunction(AstFunction const* fun) = 0;


#define DEFAULT_VISITOR(type, _) virtual void visit##type(type* node) { node->visitChildren(this); }
        FOR_NODES(DEFAULT_VISITOR)

    };


}