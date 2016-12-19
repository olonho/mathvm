#include "interpreter_code.h"

namespace mathvm {

Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
    vm executor(*this, cout);

    if (executor.run() < 0) {
        return Status::Error("");
    }

    return Status::Ok();
}

}
