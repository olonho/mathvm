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
        char const *  const name;
        Ctx ctx;
        std::ostream& _debug;
    public:

        AstAnalyzer(AstFunction const* const top, char const* name, std::ostream& debugStream)
                :  _result(new T()), top(top), name(name), _debug(debugStream) {
        }
        virtual void declareFunction(AstFunction const *fun) = 0;
        T* result() const {
            return _result;
        }
        virtual void visitAstFunction(AstFunction const* fun) = 0;


        virtual void start()  {
            declareFunction(top);

            _debug << "\n-------------------------------\n   "
                    << name << " analyzer has started \n-------------------------------\n";
            visitAstFunction(top);
        }


#define DEFAULT_VISITOR(type, _) virtual void visit##type(type* node) { node->visitChildren(this); }
        FOR_NODES(DEFAULT_VISITOR)
#undef DEFAULT_VISITOR
    };


}