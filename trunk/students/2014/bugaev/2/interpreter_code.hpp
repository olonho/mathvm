#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"


namespace mathvm
{

class InterpreterCodeImpl: public Code
{
public:
    Status *execute(vector<Var *> &vars)
    {
        return Status::Error("Executor is not implemented");
    }
};

}


#endif // INTERPRETER_CODE_HPP
