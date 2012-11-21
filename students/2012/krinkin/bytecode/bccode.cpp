#include "bccode.h"

#include "bcinterpreter.h"

Status* BCCode::execute(vector<Var*> &vars)
{
    disassemble(std::cout);
    BCInterpreter interpreter;
    interpreter.run(this);
    return 0;
}
