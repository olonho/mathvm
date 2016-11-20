#ifndef PROJECT_INTERPRETER_CODE_IMPL_H
#define PROJECT_INTERPRETER_CODE_IMPL_H

#include <mathvm.h>

namespace mathvm
{

    class InterpreterCodeImpl: public Code
    {
    public:
        Status *execute(vector<Var *> &);
    private:
    };

}

#endif //PROJECT_INTERPRETER_CODE_IMPL_H
