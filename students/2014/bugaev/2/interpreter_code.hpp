#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"


namespace mathvm
{

class InterpreterCodeImpl: public Code
{
public:
    Status *execute(vector<Var *> &);
};

}


#endif // INTERPRETER_CODE_HPP
