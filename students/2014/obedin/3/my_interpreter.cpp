#include "my_interpreter.hpp"


Status*
ICode::execute(std::vector<Var *> &vars)
{
    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        BytecodeFunction *bcFn = (BytecodeFunction*) it.next();
        std::cout << bcFn->name()
                    << "[" << bcFn->id() << "]:"
                    << std::endl;
        bcFn->bytecode()->dump(std::cout);
        std::cout << std::endl;
    }
    return Status::Ok();
}
