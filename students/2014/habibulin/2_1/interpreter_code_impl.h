#ifndef INTERPRETER_CODE_IMPL_H
#define INTERPRETER_CODE_IMPL_H

#include "mathvm.h"
#include "my_utils.h"

#include <memory>

using std::shared_ptr;

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
        return Status::Error("Code execution is not implemented, use BytecodeInterpreter instead");
    }
};

}

#endif // INTERPRETER_CODE_IMPL_H
