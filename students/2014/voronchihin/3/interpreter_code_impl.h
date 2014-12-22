#ifndef INTERPRETER_CODE_IMPL
#define INTERPRETER_CODE_IMPL

#include "mathvm.h"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    private:
        virtual Status *execute(vector<Var *> &vars);
    };
}

#endif