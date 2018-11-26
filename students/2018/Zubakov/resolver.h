#ifndef VIRTUAL_MACHINES_RESOLVER_H
#define VIRTUAL_MACHINES_RESOLVER_H

#include <visitors.h>

namespace mathvm {
    class Resolver : public AstBaseVisitor {
    private:
        Scope* currentScope;
        BytecodeFunction* currentFunction;
        Code* code;

    public:
        void visitFunction(BytecodeFunction *function);

        void visitFunctionNode(FunctionNode *node) override;
    };
}


#endif //VIRTUAL_MACHINES_RESOLVER_H
