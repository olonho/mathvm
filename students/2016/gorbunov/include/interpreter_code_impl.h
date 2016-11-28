#ifndef PROJECT_INTERPRETER_CODE_IMPL_H
#define PROJECT_INTERPRETER_CODE_IMPL_H

#include <mathvm.h>
#include <ast.h>

namespace mathvm
{

    class InterpreterCodeImpl: public Code
    {
    public:
        InterpreterCodeImpl(Scope* topScope): _top_scope(topScope) {
        }
        Status *execute(vector<Var*> &);
        uint16_t registerNativeFunction(BytecodeFunction* bf);
    private:
        Scope* _top_scope;
    };

}

#endif //PROJECT_INTERPRETER_CODE_IMPL_H
