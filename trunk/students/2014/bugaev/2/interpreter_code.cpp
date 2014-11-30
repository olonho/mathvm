#include "interpreter_code.hpp"
#include "bytecode_interpreter.hpp"


namespace mathvm
{

Status *InterpreterCodeImpl::execute(vector<Var *> &)
{
    try {
        BytecodeInterpreter(this).interpret();
    } catch (BytecodeException const &e) {
        return Status::Error(/*"Error"*/e.what());
    }
    return Status::Ok();
}

}
